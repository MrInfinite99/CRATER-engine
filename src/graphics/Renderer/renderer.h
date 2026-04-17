#pragma once
#include"../../Core/window.h"
#include "validationLayers.h"
#include "Device.h"
#include "swap_chain.h"
#include "graphics_pipeline.h"
#include"../Resources/vertex_buffer.h"
#include"../Resources/index_buffer.h"
#include"descriptor_set.h"
#include"../Resources/vma/vma_allocator.h"
#include"../Resources/textures/texture.h"
#include"../Resources/models/model.h"
#include"../constants.h"



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

			auto shaderCode = shaderCompiler.compileShader(shader_path, "vertMain", "fragMain");

			descriptorSets.reflectShader(shaderCode);
			descriptorSets.build(m_device.logicalDevice());
			m_graphicsPipeline.createPipeline(m_device.logicalDevice(),
				m_swapChain.extent(),
				m_swapChain.format(),
				m_swapChain.surfaceFormat(),
				shaderCode,
				vertexBuffer,
				descriptorSets,
				texture.findDepthFormat(m_device));

			createCommandPool();
			texture.createDepthResources(m_device,m_swapChain.extent());
			texture.createTexture(texture_path,m_allocator,m_device);
			model.load(model_path);
			vertexBuffer.createVertexBuffer(model.getVertices(),m_allocator,m_device);
			indexBuffer.createIndexBuffer(model.getIndices(),m_allocator, m_device);
			uniformBuffer.createUniformBuffer(m_allocator, m_device);

			//update descriptor set
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vk::DescriptorBufferInfo bInfo{ uniformBuffer[i].buffer(), 0, sizeof(UniformBufferObject) };
				descriptorSets.updateBuffer(m_device.logicalDevice(), i, "ubo", bInfo);

				vk::DescriptorImageInfo iInfo{ texture.sampler(), texture.view(), vk::ImageLayout::eShaderReadOnlyOptimal};
				descriptorSets.updateImage(m_device.logicalDevice(), i, "texture", iInfo);
			}


			createCommandBuffer();

			createSyncObjects();
		};

		 

		void render();

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
			//vmaDestroyAllocator(m_allocator);
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
		
		VulkanGraphicsPipeline m_graphicsPipeline{};

		vk::raii::CommandPool m_commandPool{ nullptr };

		VmaAllocatorRAII m_allocator;

		std::vector<vk::raii::CommandBuffer> commandBuffers;

		std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
		std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
		std::vector<vk::raii::Fence> inFlightFences;
		//std::optional<vk::raii::Queue> graphicsQueue;
		 
		std::optional<vk::raii::Queue> presentQueue;

		uint32_t frameIndex = 0;

		bool framebufferResized = false;
		
		ResourceManager::ShaderCompiler shaderCompiler{};
		ResourceManager::VulkanVertexBuffer vertexBuffer{};
		ResourceManager::VulkanIndexBuffer indexBuffer{};
		ResourceManager::Texture texture{};
		ResourceManager::VulkanUniformBuffer<UniformBufferObject> uniformBuffer{};
		ResourceManager::Model model;

		DescriptorSet descriptorSets{};
		
	};
}