#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "validationLayers.h"
#include "Device.h"
#include "../../Core/window.h"
#include "../Resources/vma/vma_allocator.h"
#include "../constants.h"

namespace CRATER::Renderer {

struct VulkanContext {
    ValidationLayers                 validationLayers{ true };
    Window                           window;
    vk::raii::Context                context;
    vk::raii::Instance               instance       { nullptr };
    vk::raii::DebugUtilsMessengerEXT debugMessenger { nullptr };
    vk::raii::SurfaceKHR             surface        { nullptr };
    VulkanDevice                     device;
    VmaAllocatorRAII                 allocator;

    void init();

    ~VulkanContext() {
        window.cleanUp();
    }

    VulkanContext() = default;
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
};

}
