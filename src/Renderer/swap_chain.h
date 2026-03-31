#pragma once 
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include"../CORE/window.h"
#include<vector>

namespace CRATER::Renderer {
	class VulkanSwapChain {
	public:
		void createSwapChain(vk::raii::PhysicalDevice& physicalDevice, vk::raii::SurfaceKHR& surface,Window& m_window);
		void createImageViews(vk::raii::Device& device);
	private:
		vk::raii::SwapchainKHR swapChain = nullptr;
		std::vector<vk::Image> swapChainImages;
		vk::SurfaceFormatKHR   swapChainSurfaceFormat;
		vk::Extent2D           swapChainExtent;
		std::vector<vk::raii::ImageView> swapChainImageViews;
	};
}