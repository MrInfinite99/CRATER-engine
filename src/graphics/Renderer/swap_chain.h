#pragma once 
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include"../../CORE/window.h"
 
#include<vector>
#include<iostream>


namespace CRATER::Renderer {
	class VulkanSwapChain {
	public:
		VulkanSwapChain() = default;
		~VulkanSwapChain() = default;

		void createSwapChain(vk::raii::Device& device,vk::raii::PhysicalDevice& physicalDevice, vk::raii::SurfaceKHR& surface,Window& m_window);
		void createImageViews(vk::raii::Device& device, uint32_t mipLevels);

	 

		void recreateSwapChain(vk::raii::Device& device,vk::raii::PhysicalDevice& physicalDevice, vk::raii::SurfaceKHR& surface, Window& m_window ) {
			 
			
			int width = 0, height = 0;
			SDL_GetWindowSizeInPixels(m_window.getSDLWindow(), &width, &height);

			while (width == 0 || height == 0) {
				SDL_GetWindowSizeInPixels(m_window.getSDLWindow(), &width, &height);
				SDL_WaitEvent(nullptr);  // Wait for events instead of busy-waiting
			}
			
			
			device.waitIdle();
			cleanupSwapChain();
			createSwapChain(device,physicalDevice, surface, m_window);
			createImageViews(device, 1);
			
		}

		void cleanupSwapChain() {
			swapChainImageViews.clear();
			swapChain = nullptr;
		}

		vk::Extent2D& extent() {
			return swapChainExtent;
		}

		vk::Format& format() {
			return swapChainImageFormat;
		}
		vk::SurfaceFormatKHR& surfaceFormat() {
			return swapChainSurfaceFormat;
		}

		vk::Image image(uint32_t imageIndex) {
			return swapChainImages[imageIndex];
		}

		vk::ImageView imageView(uint32_t imageIndex) {
			return swapChainImageViews[imageIndex];
		}

		vk::raii::SwapchainKHR& get() {
			return swapChain;
		}

		std::vector<vk::Image>& getSwapChainImages() {
			return swapChainImages;
		}

	private:
		vk::raii::SwapchainKHR swapChain = nullptr;
		std::vector<vk::Image> swapChainImages{};
		vk::SurfaceFormatKHR   swapChainSurfaceFormat{};
		vk::Extent2D           swapChainExtent{};
		vk::Format swapChainImageFormat = vk::Format::eUndefined;
		std::vector<vk::raii::ImageView> swapChainImageViews{};

		uint32_t mipLevels{ 0 };


	};
}