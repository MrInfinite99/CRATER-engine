#pragma once
#include"../../Core/window.h"
#include "validationLayers.h"
#include"../Resources/vma/vma_allocator.h"
#include"../Resources/resource_manager.h"
#include"../constants.h"
#include"../../Scene/Scene.h"
#include"../Resources/Object/object.h"
#include"../Resources/models/model.h"
 
 
namespace CRATER::Renderer
{
	class Renderer {
	public:
		Renderer() {
			createInstance();
			setupDebugMessenger();
			createSurface();
			// create logical device
			m_device.init(m_context, m_instance, m_surface);

			initAllocator();

			m_swapChain.createSwapChain(m_device.logicalDevice(),m_device.physicalDevice(), m_surface, m_window);
			m_swapChain.createImageViews(m_device.logicalDevice(),1);
			
			createCommandPool();
			depthTexture.createDepthResources(m_device, m_swapChain.extent());
			uniformBuffer.createUniformBuffer(m_allocator, m_device);

			createCommandBuffer();

			createSyncObjects();
		};

		void setup(CRATER::Scene::Scene& scene) {
			 
			auto view = scene.getRegistry().view<
				CRATER::Scene::TransformComponent,
				CRATER::Scene::MeshComponent,
				CRATER::Scene::MaterialComponent
				 > ();


			view.each([&](
				CRATER::Scene::TransformComponent& transform,
				CRATER::Scene::MeshComponent& meshComp,
				CRATER::Scene::MaterialComponent& materialComp) {

					auto shaderHandle = resourceManager.Load<Resource::Shader>(
						materialComp.shaderID,
						&m_device.logicalDevice(),
						&m_swapChain,
						depthTexture.getDepthFormat(),
						materialComp.type,
						materialComp.shaderID,
						"vertMain",
						"fragMain"

					);

					auto modelHandle = resourceManager.Load<Resource::Model>(
						meshComp.meshID,
						meshComp.modelPath,
						&resourceManager,
						&m_device,
						m_allocator
					);

					auto materialHandle = resourceManager.Load<Resource::Material>(
						materialComp.materialID,
						&m_device,
						shaderHandle
					);

					const auto& textures = modelHandle->GetTextures();

					Resource::RenderObject renderObj;

					for (const auto& meshHandle : modelHandle->GetMeshes()) {
						Resource::ResourceHandle<Resource::Texture> texHandle;

						const int matIdx = meshHandle->getMaterialIndex();
						if (matIdx >= 0 && matIdx < static_cast<int>(textures.size()))
							texHandle = textures[matIdx];

						renderObj.meshBindings.push_back({ meshHandle, texHandle });
					}

					

					renderObj.position = transform.position;
					renderObj.rotation = transform.rotation;
					renderObj.scale = transform.scale;

					 
					renderObj.material = materialHandle;
 
					renderObjects.push_back(std::move(renderObj));
				}
			);

			std::unordered_set<Resource::Material*> updatedMaterials;

			for (auto& obj : renderObjects) {
				auto* mat = obj.material.Get();
				for (auto& binding : obj.meshBindings) {
					for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
						mat->updateDescriptors(m_device.logicalDevice(), i, uniformBuffer, *binding.texture);
					}
				}
			}

			auto skyboxView = scene.getRegistry().view<CRATER::Scene::SkyboxComponent>();

			skyboxView.each([&](CRATER::Scene::SkyboxComponent& skyComp) {
				// Load the specialized skybox resource
				skyboxHandle = resourceManager.Load<Resource::Skybox>(
					skyComp.skyboxID,
					skyComp.skyboxPath,
					&m_device,
					&m_swapChain,
					depthTexture.getDepthFormat(),
					m_allocator,
					&resourceManager
				);

				// Immediately configure the skybox descriptor sets once loading finishes
				if (skyboxHandle.IsValid()) {
					for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
						skyboxHandle.Get()->getMaterial()->updateDescriptors(
							m_device.logicalDevice(),
							i,
							uniformBuffer,
							*skyboxHandle.Get()->getTexture()
						);
					}
				}
				});

		}

		 

		void render(CRATER::Scene::Scene& scene);

		void wait() {
			m_device.logicalDevice().waitIdle();
		}

		void cleanUp() {
			m_window.cleanUp();
 
		}

		void resized() {
			framebufferResized = true;
		}

		~Renderer() { 
		}
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void createCommandPool();
		void createCommandBuffer();
		void recordCommandBuffer(uint32_t imageIndex);
		void createSyncObjects();
		void initAllocator();
		
	
		void transition_image_layout(
			vk::Image image,
			vk::ImageLayout oldLayout,
			vk::ImageLayout newLayout,
			vk::AccessFlags2 srcAccessMask,
			vk::AccessFlags2 dstAccessMask,
			vk::PipelineStageFlags2 srcStageMask,
			vk::PipelineStageFlags2 dstStageMask,
			vk::ImageAspectFlags    image_aspect_flags
		);

		ValidationLayers validationLayers{ true };
		vk::raii::Context m_context;
		vk::raii::Instance m_instance{ nullptr };
		vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

		
		VulkanDevice m_device{};
		Window m_window{ "CRATER", WIDTH, HEIGHT };
		vk::raii::SurfaceKHR m_surface{ nullptr };
		VulkanSwapChain m_swapChain{};
	 
		vk::raii::CommandPool m_commandPool{ nullptr };

		VmaAllocatorRAII m_allocator;

		std::vector<vk::raii::CommandBuffer> commandBuffers;

		std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
		std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
		std::vector<vk::raii::Fence> inFlightFences;
 
		std::optional<vk::raii::Queue> presentQueue;

		uint32_t frameIndex = 0;

		bool framebufferResized = false;

		 
		Resource::DepthTexture depthTexture;
		Resource::VulkanUniformBuffer<UniformBufferObject> uniformBuffer{};
		PushConstant pushConstant{};
		  
		Resource::ResourceManager resourceManager;
		std::vector<Resource::RenderObject> renderObjects;
		Resource::ResourceHandle<Resource::Skybox> skyboxHandle;
	};
}