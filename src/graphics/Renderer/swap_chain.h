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

			// Block while the window is minimized / zero-sized. Pump explicitly every
			// iteration: SDL_WaitEvent(nullptr) leaves events in the queue and skips
			// pumping when the queue is non-empty — relying on it starves the OS
			// message queue and the window can never restore. Check the condition on
			// data refreshed AFTER the pump, and sleep with a timeout so no event
			// state can wedge us for more than 100ms.
			for (;;) {
				SDL_PumpEvents();   // services the OS queue — this is what lets the window restore

				int width = 0, height = 0;
				SDL_GetWindowSizeInPixels(m_window.getSDLWindow(), &width, &height);
				const bool minimized =
					(SDL_GetWindowFlags(m_window.getSDLWindow()) & SDL_WINDOW_MINIMIZED) != 0;
				if (width > 0 && height > 0 && !minimized)
					break;

				SDL_WaitEventTimeout(nullptr, 100);
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