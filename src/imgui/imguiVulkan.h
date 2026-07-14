#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <SDL3/SDL.h>
#include <imgui.h>
#include <cstdint>

namespace CRATER::Renderer { struct VulkanContext; class VulkanSwapChain; }

namespace CRATER::UI {

	// Thin wrapper over the official Dear ImGui SDL3 + Vulkan backends, configured
	// for dynamic rendering (no VkRenderPass). Lifecycle per frame:
	//   processEvent() for each SDL event  ->  beginFrame()  ->  build UI  ->
	//   endFrame() (ImGui::Render)  ->  draw(cmd) inside an open rendering scope.
	class ImGuiVulkan {
	public:
		ImGuiVulkan() = default;
		~ImGuiVulkan();

		ImGuiVulkan(const ImGuiVulkan&)            = delete;
		ImGuiVulkan& operator=(const ImGuiVulkan&) = delete;

		void init(Renderer::VulkanContext& ctx,
			Renderer::VulkanSwapChain& swapChain);
 
		void draw(vk::raii::CommandBuffer& commandBuffer);     // record draw data (rendering scope must be open)
		void beginFrame();
		void endFrame();
	  
	private:
		vk::raii::DescriptorPool m_descriptorPool{ nullptr };
	 
		bool     m_initialized = false;
	};
}
