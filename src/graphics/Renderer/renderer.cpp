#include "renderer.h"

namespace CRATER::Renderer
{
	 

	void Renderer::createInstance() {
		
		validationLayers.enable(true);

		constexpr vk::ApplicationInfo appInfo{
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14 };

		std::vector<const char*> requiredLayers = validationLayers.getRequiredLayers(m_context);
		std::vector<const char*> extensionsList = validationLayers.getExtensionList(m_context);


		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames = requiredLayers.data(),
			 .enabledExtensionCount = static_cast<uint32_t>(extensionsList.size()),
		.ppEnabledExtensionNames = extensionsList.data()
		};

		m_instance = vk::raii::Instance(m_context, createInfo);
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
		std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

		return vk::False;
	}

	void Renderer::setupDebugMessenger() {
		if (!validationLayers.isEnabled()) {
			return;
		}

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			.messageSeverity = severityFlags,
			.messageType = messageTypeFlags,
			.pfnUserCallback = &debugCallback
		};
		debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void Renderer::createSurface() {
		VkSurfaceKHR _surface = VK_NULL_HANDLE;

		if (!SDL_Vulkan_CreateSurface(m_window.getSDLWindow(), *m_instance, nullptr, &_surface)) {
			throw std::runtime_error("Failed to create Vulkan surface from Sdl window");
		}

		m_surface = vk::raii::SurfaceKHR(m_instance, _surface);
	}

	void Renderer::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	.queueFamilyIndex = m_device.graphicsIndex()};

		m_commandPool = vk::raii::CommandPool(m_device.logicalDevice(), poolInfo);
	}

	void Renderer::createCommandBuffer() {
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = m_commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT };

		commandBuffers = vk::raii::CommandBuffers(m_device.logicalDevice(), allocInfo);
	}

	void Renderer::recordCommandBuffer(uint32_t imageIndex) {

		auto& commandBuffer = commandBuffers[frameIndex];

		commandBuffer.begin({});
		// Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
		transition_image_layout(
			m_swapChain.image(imageIndex),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},                                                         // srcAccessMask (no need to wait for previous operations)
			vk::AccessFlagBits2::eColorAttachmentWrite,                 // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // srcStage
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,	        // dstStage
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
			vk::ImageAspectFlagBits::eDepth);

		vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderingAttachmentInfo attachmentInfo = {
			.imageView = m_swapChain.imageView(imageIndex),
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = clearColor
		};

		vk::RenderingAttachmentInfo depthAttachmentInfo = {
	.imageView = depthTexture.depthimageview(),
	.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
	.loadOp = vk::AttachmentLoadOp::eClear,
	.storeOp = vk::AttachmentStoreOp::eDontCare,
	.clearValue = clearDepth };

		vk::RenderingInfo renderingInfo = {
		.renderArea = {.offset = { 0, 0 }, .extent = m_swapChain.extent()},
	.layerCount = 1,
	.colorAttachmentCount = 1,
	.pColorAttachments = &attachmentInfo,
	.pDepthAttachment=&depthAttachmentInfo
		};

		commandBuffer.beginRendering(renderingInfo);

		 
		for (auto& obj : renderObjects) {

			 
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


				 
			
			 
			commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapChain.extent().width), static_cast<float>(m_swapChain.extent().height), 0.0f, 1.0f));
			commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChain.extent()));


			for (auto& binding : obj.meshBindings) {
				commandBuffer.bindVertexBuffers(
					0,
					static_cast<vk::Buffer>(binding.mesh.Get()->getVertexBuffer().get()),
					{ 0 }
				);


				commandBuffer.bindIndexBuffer(
					static_cast<vk::Buffer>(binding.mesh.Get()->getIndexBuffer().get()),
					0,
					vk::IndexType::eUint32
				);
		 }
				 
				
			

			StandardPushConstants pc{};
			pc.model = obj.getModelMatrix();
			commandBuffer.pushConstants<StandardPushConstants>(
				*obj.material.Get()->getShader()->layout().layout(),
				vk::ShaderStageFlagBits::eVertex,  // ✓ Match the range
				0,
				pc
			);

			for (auto& binding : obj.meshBindings) {
				commandBuffer.drawIndexed(
					static_cast<uint32_t>(binding.mesh.Get()->getIndexBuffer().getIndicesSize()),
					1,
					0,
					0,
					0
				);
			}
			
		}

		if (skyboxHandle.IsValid()) {
			auto* skybox = skyboxHandle.Get();
			auto* skyMaterial = skybox->getMaterial();
			 

			// Bind the specialized skybox pipeline (Depth Write: OFF, Depth Compare: LESS_OR_EQUAL)
			commandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				skyMaterial->getShader()->pipeline().pipeline()
			);

			// Bind the skybox descriptor set containing your UBO and SamplerCube
			commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*skyMaterial->getShader()->layout().layout(),
				0,
				*skyMaterial->getDescriptorSet()[frameIndex],
				nullptr
			);

			// Set viewport/scissor to match the frame configuration
			commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapChain.extent().width), static_cast<float>(m_swapChain.extent().height), 0.0f, 1.0f));
			commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChain.extent()));

			// Bind the hardcoded 1x1x1 cube geometry buffers
			commandBuffer.bindVertexBuffers(
				0,
				static_cast<vk::Buffer>(skybox->getVertexBuffer().get()),
				{ 0 }
			);

			commandBuffer.bindIndexBuffer(
				static_cast<vk::Buffer>(skybox->getIndexBuffer().get()),
				0,
				vk::IndexType::eUint32
			);

			// Issue the skybox draw call (Will automatically render behind everything else)
			commandBuffer.drawIndexed(
				static_cast<uint32_t>(skybox->getIndexBuffer().getIndicesSize()),
				1,
				0,
				0,
				0
			);
		}
		
		commandBuffer.endRendering();

		// After rendering, transition the swapchain image to PRESENT_SRC
		transition_image_layout(
			m_swapChain.image(imageIndex),
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
			{},                                                     // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
			vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
			vk::ImageAspectFlagBits::eColor
		);

		commandBuffer.end();

	}

	void Renderer::transition_image_layout(
		vk::Image image,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::AccessFlags2 srcAccessMask,
		vk::AccessFlags2 dstAccessMask,
		vk::PipelineStageFlags2 srcStageMask,
		vk::PipelineStageFlags2 dstStageMask,
		vk::ImageAspectFlags    image_aspect_flags
	) {
		 
		vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask = srcStageMask,
			.srcAccessMask = srcAccessMask,
			.dstStageMask = dstStageMask,
			.dstAccessMask = dstAccessMask,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
		.subresourceRange = {
			.aspectMask = image_aspect_flags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1} };
		vk::DependencyInfo dependencyInfo = {
			.dependencyFlags = {},
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier
		};
		commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
	}

	void Renderer::createSyncObjects() {
		 
			assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

			for (size_t i = 0; i < m_swapChain.getSwapChainImages().size(); i++)
			{
				renderFinishedSemaphores.emplace_back(m_device.logicalDevice(), vk::SemaphoreCreateInfo());
			}

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				presentCompleteSemaphores.emplace_back(m_device.logicalDevice(), vk::SemaphoreCreateInfo());
				inFlightFences.emplace_back(m_device.logicalDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
			}
	 
	}

	void Renderer::render(CRATER::Scene::Scene& scene) {
		
		auto fenceResult = m_device.logicalDevice().waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

		

		auto [result, imageIndex] = m_swapChain.get().acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
		 

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			 
			m_swapChain.recreateSwapChain(m_device.logicalDevice(), m_device.physicalDevice(), m_surface, m_window);
			depthTexture.createDepthResources(m_device, m_swapChain.extent());
			return;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			 
			assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		m_device.logicalDevice().resetFences(*inFlightFences[frameIndex]);

		commandBuffers[frameIndex].reset();
		recordCommandBuffer(imageIndex);
		/*****************************************TEMP*****************************************************************/
		 

			UniformBufferObject ubo{};
			  
			ubo.view = scene.getCamera().getViewMatrix();
			ubo.proj = scene.getCamera().getProjectionMatrix(static_cast<float>(m_swapChain.extent().width) / static_cast<float>(m_swapChain.extent().height), 0.1f, 100.0f);
			ubo.proj[1][1] *= -1;
		 
		/******************************************************************************************************************/
		uniformBuffer.updateUniformBuffer(frameIndex, ubo);
		 

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
			.pWaitDstStageMask = &waitDestinationStageMask,
			.commandBufferCount = 1,
			.pCommandBuffers = &*commandBuffers[frameIndex],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] };

		m_device.queue()->submit(submitInfo, *inFlightFences[frameIndex]);
		 
		const vk::PresentInfoKHR presentInfoKHR{
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
	.swapchainCount = 1,
	.pSwapchains = &*m_swapChain.get(),
	.pImageIndices = &imageIndex };

		try {
			 
			result = m_device.presentqueue()->presentKHR(presentInfoKHR);
		}
		catch (const vk::OutOfDateKHRError& e) {
			 
			result = vk::Result::eErrorOutOfDateKHR;
		}
		catch (const vk::SystemError& e) {
			 
			throw std::runtime_error("Vulkan system error during present!");
		}
		 
		if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
		{
			framebufferResized = false;
			//std::cout << "AAAAAAAAAAA" << std::endl;
			m_swapChain.recreateSwapChain(m_device.logicalDevice(),m_device.physicalDevice(), m_surface, m_window);
			depthTexture.createDepthResources(m_device, m_swapChain.extent());
		}
		else
		{
			//std::cout << "AAAAAAAAAAA" << std::endl;
			// There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
			assert(result == vk::Result::eSuccess);
		}
		frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::initAllocator() {

		VmaVulkanFunctions vmaFunctions = {};
		vmaFunctions.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)m_instance.getDispatcher()->vkGetInstanceProcAddr;
		vmaFunctions.vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)m_device.logicalDevice().getDispatcher()->vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorInfo.physicalDevice = *m_device.physicalDevice();
		allocatorInfo.device = *m_device.logicalDevice();
		allocatorInfo.instance = *m_instance;
		allocatorInfo.pVulkanFunctions = &vmaFunctions;


		 
		m_allocator = VmaAllocatorRAII(allocatorInfo);
	}

	 

	 

	 /*std::vector<std::string> skyboxTexPath = {
				"D:/vkguide/VkRE/textures/skybox/right.jpg",
				"D:/vkguide/VkRE/textures/skybox/left.jpg",
				"D:/vkguide/VkRE/textures/skybox/top.jpg",
				"D:/vkguide/VkRE/textures/skybox/bottom.jpg",
				"D:/vkguide/VkRE/textures/skybox/front.jpg",
				"D:/vkguide/VkRE/textures/skybox/back.jpg"
			};*/
}          