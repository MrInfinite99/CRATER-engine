#include "vulkanContext.h"
#include <SDL3/SDL_vulkan.h>
#include"../../Core/logger.h"

namespace CRATER::Renderer {

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*)
{
   
    Log::Level lvl{};
    
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        lvl = Log::Level::Error;
    }
    else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        lvl = Log::Level::Info;
    }
    else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        lvl = Log::Level::Warn;
    }
    Log::write(lvl, "validation layer : type + " + to_string(type) + pCallbackData->pMessage);
               
    return vk::False;
}

void VulkanContext::init() {
    // Window
    window.init("CRATER", WIDTH, HEIGHT);

    // Instance
    validationLayers.enable(true);
    std::vector<const char*> requiredLayers    = validationLayers.getRequiredLayers(context);
    std::vector<const char*> extensionsList    = validationLayers.getExtensionList(context);

    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName   = "CRATER",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14
    };

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames     = requiredLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(extensionsList.size()),
        .ppEnabledExtensionNames = extensionsList.data()
    };

    instance = vk::raii::Instance(context, createInfo);

    // Debug messenger
    if (validationLayers.isEnabled()) {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        debugMessenger = instance.createDebugUtilsMessengerEXT({
            .messageSeverity = severityFlags,
            .messageType     = messageTypeFlags,
            .pfnUserCallback = &debugCallback
        });
    }

    // Surface
    VkSurfaceKHR raw = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window.getSDLWindow(), *instance, nullptr, &raw))
        throw std::runtime_error("Failed to create Vulkan surface");
    surface = vk::raii::SurfaceKHR(instance, raw);

    // Device
    device.init(context, instance, surface);

    // Allocator
    VmaVulkanFunctions vmaFunctions{};
    vmaFunctions.vkGetInstanceProcAddr =
        (PFN_vkGetInstanceProcAddr)instance.getDispatcher()->vkGetInstanceProcAddr;
    vmaFunctions.vkGetDeviceProcAddr =
        (PFN_vkGetDeviceProcAddr)device.logicalDevice().getDispatcher()->vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    allocatorInfo.physicalDevice   = *device.physicalDevice();
    allocatorInfo.device           = *device.logicalDevice();
    allocatorInfo.instance         = *instance;
    allocatorInfo.pVulkanFunctions = &vmaFunctions;

    allocator = VmaAllocatorRAII(allocatorInfo);
}

}
