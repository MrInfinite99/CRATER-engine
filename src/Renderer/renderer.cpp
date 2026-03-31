#include "renderer.h"

namespace CRATER::Renderer
{
	void Renderer::init() {
		m_window.init("CRATER", WIDTH, HEIGHT);

        createInstance();
		setupDebugMessenger();
		createSurface();
		// create logical device
		m_device.init(m_context, m_instance,m_surface);

		m_swapChain.createSwapChain(m_device.physicalDevice(), m_surface, m_window);
		m_swapChain.createImageViews(m_device.logicalDevice());
		
	}

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
}         