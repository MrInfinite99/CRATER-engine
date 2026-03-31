#pragma once
#include"../Core/window.h"
#include "validationLayers.h"
#include "Device.h"
#include "swap_chain.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

namespace CRATER::Renderer
{
	class Renderer {
	public:
		void init();
	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();

		vk::raii::Context m_context;
		vk::raii::Instance m_instance{ nullptr };
		vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

		ValidationLayers validationLayers;
		VulkanDevice m_device;
		Window m_window;
		VulkanSwapChain m_swapChain;
		vk::raii::SurfaceKHR m_surface{ nullptr };
	};
}