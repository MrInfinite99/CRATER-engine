#include "renderer.h"
#include "../Resources/textures/texture.h"
#include <ktx.h>

namespace CRATER::Renderer
{
	static void createFallbackTexture(
		Resource::ResourceManager& rm,
		const std::string& id,
		uint8_t r, uint8_t g, uint8_t b, uint8_t a,
		VmaAllocator allocator,
		VulkanDevice* device)
	{
		const uint8_t pixel[4] = { r, g, b, a };

		ktxTextureCreateInfo info{};
		info.vkFormat        = VK_FORMAT_R8G8B8A8_SRGB;
		info.baseWidth       = 1;
		info.baseHeight      = 1;
		info.baseDepth       = 1;
		info.numDimensions   = 2;
		info.numLevels       = 1;
		info.numLayers       = 1;
		info.numFaces        = 1;
		info.isArray         = KTX_FALSE;
		info.generateMipmaps = KTX_FALSE;

		ktxTexture2* ktx = nullptr;
		if (ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx) != KTX_SUCCESS) {
			std::cerr << "[Renderer] Failed to create fallback texture: " << id << "\n";
			return;
		}
		ktxTexture_SetImageFromMemory(ktxTexture(ktx), 0, 0, 0, pixel, 4);

		Resource::TextureData td{};
		td.ktx    = ktx;
		td.width  = 1;
		td.height = 1;
		td.levels = 1;
		td.layers = 1;
		td.format = vk::Format::eR8G8B8A8Srgb;
		td.name   = id;

		rm.Load<Resource::Texture>(id, td, allocator, device);
	}

	Renderer::Renderer(VulkanContext& ctx) : m_ctx(ctx) {
		slang::createGlobalSession(m_slangSession.writeRef());

		m_swapChain.createSwapChain(m_ctx.device.logicalDevice(), m_ctx.device.physicalDevice(), m_ctx.surface, m_ctx.window);
		m_swapChain.createImageViews(m_ctx.device.logicalDevice(), 1);

		createCommandPool();
		depthTexture.createDepthResources(m_ctx.device, m_swapChain.extent());
		uniformBuffer.createUniformBuffer(m_ctx.allocator, m_ctx.device);

		createCommandBuffer();
		createSyncObjects();

		createFallbackTexture(resourceManager, "__fallback_white",  255, 255, 255, 255, m_ctx.allocator, &m_ctx.device);
		createFallbackTexture(resourceManager, "__fallback_normal", 128, 128, 255, 255, m_ctx.allocator, &m_ctx.device);
		createFallbackTexture(resourceManager, "__fallback_black",    0,   0,   0, 255, m_ctx.allocator, &m_ctx.device);
	}

	void Renderer::setup(CRATER::Scene::Scene& scene) {
		auto view = scene.getRegistry().view<
			CRATER::Scene::TransformComponent,
			CRATER::Scene::MeshComponent,
			CRATER::Scene::MaterialComponent>();

		view.each([&](
			entt::entity entity,
			CRATER::Scene::TransformComponent& transform,
			CRATER::Scene::MeshComponent& meshComp,
			CRATER::Scene::MaterialComponent& materialComp)
		{
			auto shaderHandle = resourceManager.Load<Resource::Shader>(
				materialComp.shaderID,
				&m_ctx.device.logicalDevice(),
				&m_swapChain,
				depthTexture.getDepthFormat(),
				materialComp.type,
				m_slangSession.get(),
				materialComp.shaderID,
				"vertMain",
				"fragMain"
			);

			auto modelHandle = resourceManager.Load<Resource::Model>(
				meshComp.meshID,
				meshComp.modelPath,
				&resourceManager,
				&m_ctx.device,
				m_ctx.allocator
			);

			if (!modelHandle.IsValid()) {
				std::cerr << "[Renderer] Failed to load model: " << meshComp.modelPath << "\n";
				return;
			}

			auto materialHandle = resourceManager.Load<Resource::Material>(
				materialComp.materialID,
				&m_ctx.device,
				shaderHandle
			);

			const auto& textures     = modelHandle->GetTextures();
			const auto& materialData = modelHandle->GetMaterials();

			Resource::RenderObject renderObj;

			for (const auto& meshHandle : modelHandle->GetMeshes()) {
				if (!meshHandle.IsValid()) continue;

				const int matIdx = meshHandle->getMaterialIndex();

				Resource::ResourceHandle<Resource::Texture> albedoHandle;
				Resource::ResourceHandle<Resource::Texture> metallicRoughnessHandle;
				Resource::ResourceHandle<Resource::Texture> normalHandle;
				Resource::ResourceHandle<Resource::Texture> occlusionHandle;
				Resource::ResourceHandle<Resource::Texture> emissiveHandle;
				Resource::MaterialData md{};

				if (matIdx >= 0 && matIdx < static_cast<int>(materialData.size()) && !materialData.empty()) {
					const auto& tex = materialData[matIdx];
					md = tex;

					auto safeTexture = [&](int idx, const char* fallbackId)
						-> Resource::ResourceHandle<Resource::Texture>
					{
						if (idx >= 0 && idx < static_cast<int>(textures.size()) && textures[idx].IsValid())
							return textures[idx];
						return resourceManager.GetHandle<Resource::Texture>(fallbackId);
					};

					albedoHandle            = safeTexture(tex.albedoIndex,            "__fallback_white");
					metallicRoughnessHandle = safeTexture(tex.metallicRoughnessIndex, "__fallback_white");
					normalHandle            = safeTexture(tex.normalIndex,            "__fallback_normal");
					occlusionHandle         = safeTexture(tex.occlusionIndex,         "__fallback_white");
					emissiveHandle          = safeTexture(tex.emissiveIndex,          "__fallback_black");
				}

				// Any slot still invalid (no material at all) gets a fallback
				if (!albedoHandle.IsValid())
					albedoHandle = resourceManager.GetHandle<Resource::Texture>("__fallback_white");
				if (!metallicRoughnessHandle.IsValid())
					metallicRoughnessHandle = resourceManager.GetHandle<Resource::Texture>("__fallback_white");
				if (!normalHandle.IsValid())
					normalHandle = resourceManager.GetHandle<Resource::Texture>("__fallback_normal");
				if (!occlusionHandle.IsValid())
					occlusionHandle = resourceManager.GetHandle<Resource::Texture>("__fallback_white");
				if (!emissiveHandle.IsValid())
					emissiveHandle = resourceManager.GetHandle<Resource::Texture>("__fallback_black");

				renderObj.meshBindings.push_back({
					meshHandle,
					albedoHandle,
					metallicRoughnessHandle,
					normalHandle,
					occlusionHandle,
					emissiveHandle,
					md
				});
			}

			renderObj.position = transform.position;
			renderObj.rotation = transform.rotation;
			renderObj.scale    = transform.scale;
			renderObj.material = materialHandle;

			renderObjects.emplace(entity, std::move(renderObj));
		});

		std::unordered_set<Resource::Material*> updatedMaterials;
		for (auto&& [entity, obj] : renderObjects.each()) {
			auto* mat = obj.material.Get();
			if (!mat) continue;
			for (auto& binding : obj.meshBindings) {
				if (updatedMaterials.find(mat) == updatedMaterials.end()) {
					for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
						mat->updateDescriptors(m_ctx.device.logicalDevice(), i, uniformBuffer,
							*binding.albedo,
							*binding.metallicRoughness,
							*binding.normal,
							*binding.occlusion,
							*binding.emissive);
					}
					updatedMaterials.insert(mat);
				}
			}
		}

		auto skyboxView = scene.getRegistry().view<CRATER::Scene::SkyboxComponent>();
		skyboxView.each([&](CRATER::Scene::SkyboxComponent& skyComp) {
			skyboxHandle = resourceManager.Load<Resource::Skybox>(
				skyComp.skyboxID,
				skyComp.skyboxPath,
				&m_ctx.device,
				&m_swapChain,
				depthTexture.getDepthFormat(),
				m_ctx.allocator,
				&resourceManager,
				m_slangSession.get()
			);

			if (skyboxHandle.IsValid()) {
				for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
					skyboxHandle.Get()->getMaterial()->updateDescriptors(
						m_ctx.device.logicalDevice(), i, uniformBuffer,
						*skyboxHandle.Get()->getTexture()
					);
				}
			}
		});
	}

	void Renderer::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{
			.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = m_ctx.device.graphicsIndex()
		};
		m_commandPool = vk::raii::CommandPool(m_ctx.device.logicalDevice(), poolInfo);
	}

	void Renderer::createCommandBuffer() {
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool        = m_commandPool,
			.level              = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT
		};
		commandBuffers = vk::raii::CommandBuffers(m_ctx.device.logicalDevice(), allocInfo);
	}

	void Renderer::recordCommandBuffer(uint32_t imageIndex) {
		auto& commandBuffer = commandBuffers[frameIndex];
		commandBuffer.begin({});

		transition_image_layout(
			m_swapChain.image(imageIndex),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor
		);

		transition_image_layout(
			depthTexture.depthimage(),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthAttachmentOptimal,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::ImageAspectFlagBits::eDepth
		);

		vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingAttachmentInfo attachmentInfo{
			.imageView   = m_swapChain.imageView(imageIndex),
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp      = vk::AttachmentLoadOp::eClear,
			.storeOp     = vk::AttachmentStoreOp::eStore,
			.clearValue  = clearColor
		};

		vk::RenderingAttachmentInfo depthAttachmentInfo{
			.imageView   = depthTexture.depthimageview(),
			.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
			.loadOp      = vk::AttachmentLoadOp::eClear,
			.storeOp     = vk::AttachmentStoreOp::eDontCare,
			.clearValue  = clearDepth
		};

		vk::RenderingInfo renderingInfo{
			.renderArea            = {.offset = {0, 0}, .extent = m_swapChain.extent()},
			.layerCount            = 1,
			.colorAttachmentCount  = 1,
			.pColorAttachments     = &attachmentInfo,
			.pDepthAttachment      = &depthAttachmentInfo
		};

		commandBuffer.beginRendering(renderingInfo);

		// Passes record into the already-open dynamic-rendering scope. They share the
		// same color+depth attachments, so no barrier is needed between them. When this
		// migrates to a framegraph, these bodies become pass execute() callbacks and the
		// graph takes over beginRendering/transitions.
		recordOpaquePass(commandBuffer);
		recordSkyboxPass(commandBuffer);

		commandBuffer.endRendering();

		transition_image_layout(
			m_swapChain.image(imageIndex),
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite,
			{},
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eBottomOfPipe,
			vk::ImageAspectFlagBits::eColor
		);

		commandBuffer.end();
	}

	void Renderer::recordOpaquePass(vk::raii::CommandBuffer& commandBuffer) {
		for (auto&& [entity, obj] : renderObjects.each()) {
			commandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				obj.material.Get()->getShader()->pipeline().pipeline()
			);
			commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*obj.material.Get()->getShader()->layout().layout(),
				0,
				*obj.material.Get()->getDescriptorSet()[frameIndex],
				nullptr
			);
			commandBuffer.setViewport(0, vk::Viewport(
				0.0f, 0.0f,
				static_cast<float>(m_swapChain.extent().width),
				static_cast<float>(m_swapChain.extent().height),
				0.0f, 1.0f
			));
			commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChain.extent()));

			for (auto& binding : obj.meshBindings) {
				commandBuffer.bindVertexBuffers(0,
					static_cast<vk::Buffer>(binding.mesh.Get()->getVertexBuffer().get()), {0});
				commandBuffer.bindIndexBuffer(
					static_cast<vk::Buffer>(binding.mesh.Get()->getIndexBuffer().get()),
					0, vk::IndexType::eUint32);

				StandardPushConstants pc{};
				pc.model            = obj.getModelMatrix();
				pc.baseColorFactor  = binding.materialData.baseColorFactor;
				pc.metallicFactor   = binding.materialData.metallicFactor;
				pc.roughnessFactor  = binding.materialData.roughnessFactor;
				pc.alphaMaskCutoff  = binding.materialData.alphaCutoff;
				commandBuffer.pushConstants<StandardPushConstants>(
					*obj.material.Get()->getShader()->layout().layout(),
					vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					0, pc
				);

				commandBuffer.drawIndexed(
					static_cast<uint32_t>(binding.mesh.Get()->getIndexBuffer().getIndicesSize()),
					1, 0, 0, 0
				);
			}
		}
	}

	void Renderer::recordSkyboxPass(vk::raii::CommandBuffer& commandBuffer) {
		if (!skyboxHandle.IsValid()) return;

		auto* skybox = skyboxHandle.Get();
		auto* skyMat = skybox->getMaterial();

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
			skyMat->getShader()->pipeline().pipeline());
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			*skyMat->getShader()->layout().layout(), 0,
			*skyMat->getDescriptorSet()[frameIndex], nullptr);
		commandBuffer.setViewport(0, vk::Viewport(
			0.0f, 0.0f,
			static_cast<float>(m_swapChain.extent().width),
			static_cast<float>(m_swapChain.extent().height),
			0.0f, 1.0f
		));
		commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChain.extent()));
		commandBuffer.bindVertexBuffers(0,
			static_cast<vk::Buffer>(skybox->getVertexBuffer().get()), {0});
		commandBuffer.bindIndexBuffer(
			static_cast<vk::Buffer>(skybox->getIndexBuffer().get()),
			0, vk::IndexType::eUint32);
		commandBuffer.drawIndexed(
			static_cast<uint32_t>(skybox->getIndexBuffer().getIndicesSize()),
			1, 0, 0, 0
		);
	}

	void Renderer::transition_image_layout(
		vk::Image image,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::AccessFlags2 srcAccessMask,
		vk::AccessFlags2 dstAccessMask,
		vk::PipelineStageFlags2 srcStageMask,
		vk::PipelineStageFlags2 dstStageMask,
		vk::ImageAspectFlags image_aspect_flags)
	{
		vk::ImageMemoryBarrier2 barrier{
			.srcStageMask  = srcStageMask,
			.srcAccessMask = srcAccessMask,
			.dstStageMask  = dstStageMask,
			.dstAccessMask = dstAccessMask,
			.oldLayout     = oldLayout,
			.newLayout     = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image         = image,
			.subresourceRange = {
				.aspectMask     = image_aspect_flags,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1
			}
		};
		commandBuffers[frameIndex].pipelineBarrier2(vk::DependencyInfo{
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers    = &barrier
		});
	}

	void Renderer::createSyncObjects() {
		assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

		for (size_t i = 0; i < m_swapChain.getSwapChainImages().size(); i++)
			renderFinishedSemaphores.emplace_back(m_ctx.device.logicalDevice(), vk::SemaphoreCreateInfo());

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			presentCompleteSemaphores.emplace_back(m_ctx.device.logicalDevice(), vk::SemaphoreCreateInfo());
			inFlightFences.emplace_back(m_ctx.device.logicalDevice(),
				vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
		}
	}

	void Renderer::sync(CRATER::Scene::Scene& scene) {
		// ── Camera → per-frame UBO data (uploaded to the GPU in render()) ──────
		auto& camera = scene.getCamera();
		const float aspect =
			static_cast<float>(m_swapChain.extent().width) /
			static_cast<float>(m_swapChain.extent().height);

		m_ubo.view   = camera.getViewMatrix();
		m_ubo.proj   = camera.getProjectionMatrix(aspect, 0.1f, 100.0f);
		m_ubo.proj[1][1] *= -1;              // Vulkan clip-space Y points down
		m_ubo.camPos = camera.getPosition();

		// ── Refresh per-object transforms from the ECS ─────────────────────────
		// renderObjects is a sparse set keyed by entity, so each RenderObject carries
		// its own key — no parallel list to keep aligned.
		auto& registry = scene.getRegistry();
		for (auto&& [entity, ro] : renderObjects.each()) {
			if (!registry.valid(entity)) continue;

			if (auto* t = registry.try_get<CRATER::Scene::TransformComponent>(entity)) {
				ro.position = t->GetPosition();
				ro.rotation = t->GetRotation();
				ro.scale    = t->GetScale();
			}
		}
	}

	void Renderer::render(CRATER::Scene::Scene& scene) {
		auto fenceResult = m_ctx.device.logicalDevice().waitForFences(
			*inFlightFences[frameIndex], vk::True, UINT64_MAX);

		auto [result, imageIndex] = m_swapChain.get().acquireNextImage(
			UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR) {
			m_swapChain.recreateSwapChain(m_ctx.device.logicalDevice(), m_ctx.device.physicalDevice(), m_ctx.surface, m_ctx.window);
			depthTexture.createDepthResources(m_ctx.device, m_swapChain.extent());
			return;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
			throw std::runtime_error("Failed to acquire swap chain image");

		m_ctx.device.logicalDevice().resetFences(*inFlightFences[frameIndex]);
		commandBuffers[frameIndex].reset();
		recordCommandBuffer(imageIndex);

		// Camera/transform data was gathered in sync(); just upload it for this frame.
		uniformBuffer.updateUniformBuffer(frameIndex, m_ubo);

		vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		m_ctx.device.queue()->submit(vk::SubmitInfo{
			.waitSemaphoreCount   = 1,
			.pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
			.pWaitDstStageMask    = &waitStage,
			.commandBufferCount   = 1,
			.pCommandBuffers      = &*commandBuffers[frameIndex],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
		}, *inFlightFences[frameIndex]);

		try {
			result = m_ctx.device.presentqueue()->presentKHR(vk::PresentInfoKHR{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
				.swapchainCount     = 1,
				.pSwapchains        = &*m_swapChain.get(),
				.pImageIndices      = &imageIndex
			});
		}
		catch (const vk::OutOfDateKHRError&) {
			result = vk::Result::eErrorOutOfDateKHR;
		}
		catch (const vk::SystemError&) {
			throw std::runtime_error("Vulkan system error during present");
		}

		if (result == vk::Result::eSuboptimalKHR ||
		    result == vk::Result::eErrorOutOfDateKHR ||
		    framebufferResized)
		{
			framebufferResized = false;
			m_swapChain.recreateSwapChain(m_ctx.device.logicalDevice(), m_ctx.device.physicalDevice(), m_ctx.surface, m_ctx.window);
			depthTexture.createDepthResources(m_ctx.device, m_swapChain.extent());
		}
		else {
			assert(result == vk::Result::eSuccess);
		}

		frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}
}
