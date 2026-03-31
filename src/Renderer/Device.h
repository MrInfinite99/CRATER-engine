#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "validationLayers.h"
#include <optional>
#include<map>

namespace CRATER::Renderer
{
    class VulkanDevice {
    public:
        VulkanDevice() = default;
        ~VulkanDevice() = default;

        // Initialize the logical device and select a physical device with a graphics queue
        void init(vk::raii::Context& context, vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface);
        void pickPhysicalDevice(vk::raii::Instance& instance);
        void createLogicalDevice(vk::raii::SurfaceKHR& surface);

        vk::raii::PhysicalDevice& physicalDevice() { return m_physicalDevice; }
        vk::raii::Device& logicalDevice() { return m_device; }
        vk::raii::Queue& queue() { return m_queueIndex; }
        uint32_t graphicsIndex() const { return m_graphicsIndex; }

        vk::raii::CommandPool& commandPool() { return m_commandPool; }

        void waitIdle() { if (m_device) m_device.waitIdle(); }

    private:
        vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
        vk::raii::Device m_device{ nullptr };
        vk::raii::Queue m_queueIndex {nullptr };
        uint32_t m_graphicsIndex{ 0 };
        vk::raii::CommandPool m_commandPool{ nullptr };
    };

}
