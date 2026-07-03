#pragma once
#include "vulkanContext.h"
#include "../Resources/resource_manager.h"
#include "../constants.h"
#include "../../Scene/Scene.h"
#include "../Resources/Object/object.h"
#include "../Resources/models/model.h"

#include <slang.h>
#include <slang-com-ptr.h>
#include <entt/entt.hpp>

namespace CRATER::Renderer
{
	class Renderer {
	public:
		explicit Renderer(VulkanContext& ctx);

		void setup(CRATER::Scene::Scene& scene);
		void sync(CRATER::Scene::Scene& scene);
		void render(CRATER::Scene::Scene& scene);

		void wait()    { m_ctx.device.logicalDevice().waitIdle(); }
		void resized() { framebufferResized = true; }

		~Renderer() {}
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	private:
		void createCommandPool();
		void createCommandBuffer();
		void recordCommandBuffer(uint32_t imageIndex);
		void recordOpaquePass(vk::raii::CommandBuffer& commandBuffer);
		void recordSkyboxPass(vk::raii::CommandBuffer& commandBuffer);
		void createSyncObjects();

		void transition_image_layout(
			vk::Image image,
			vk::ImageLayout oldLayout,
			vk::ImageLayout newLayout,
			vk::AccessFlags2 srcAccessMask,
			vk::AccessFlags2 dstAccessMask,
			vk::PipelineStageFlags2 srcStageMask,
			vk::PipelineStageFlags2 dstStageMask,
			vk::ImageAspectFlags image_aspect_flags
		);

		VulkanContext& m_ctx;

		VulkanSwapChain m_swapChain{};

		vk::raii::CommandPool m_commandPool{ nullptr };
		std::vector<vk::raii::CommandBuffer> commandBuffers;

		std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
		std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
		std::vector<vk::raii::Fence>     inFlightFences;

		std::optional<vk::raii::Queue> presentQueue;

		uint32_t frameIndex = 0;
		bool framebufferResized = false;

		Slang::ComPtr<slang::IGlobalSession> m_slangSession;

		Resource::DepthTexture depthTexture;
		Resource::VulkanUniformBuffer<UniformBufferObject> uniformBuffer{};
		PushConstant pushConstant{};

		Resource::ResourceManager resourceManager;
		entt::storage<Resource::RenderObject> renderObjects;   // sparse set keyed by entt::entity; O(1) insert/erase, packed iteration
		Resource::ResourceHandle<Resource::Skybox> skyboxHandle;

		UniformBufferObject m_ubo{};   // CPU-side per-frame camera data, populated by sync(), uploaded in render()
	};
}
