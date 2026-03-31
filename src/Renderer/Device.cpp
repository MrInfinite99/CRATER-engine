#include "Device.h"
#include <stdexcept>
#include <set>

namespace CRATER::Renderer {

    void VulkanDevice::init(vk::raii::Context& context, vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface) {
        // Select a suitable physical device (first discrete GPU or first available)
        pickPhysicalDevice(instance);
        // Find queue family with graphics support
        createLogicalDevice(surface);

        m_graphicsQueue = vk::raii::Queue(m_device, m_queueIndex, 0);

        m_commandPool = vk::raii::CommandPool(m_device, vk::CommandPoolCreateInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_queueIndex });
    }

    void VulkanDevice::pickPhysicalDevice(vk::raii::Instance& instance) {
        auto physicalDevices = vk::raii::PhysicalDevices(instance);
        if (physicalDevices.empty())
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<int, vk::raii::PhysicalDevice> candidates;

        for (const auto& pd : physicalDevices)
        {
            uint32_t score = 0;
            bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
            if (supportsVulkan1_3) {
                score += 1000;
            }

            auto queueFamilies = physicalDevice.getQueueFamilyProperties();
            bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

            if (supportsGraphics) {
                score += 1000;
            }
           
            auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
            bool supportsAllRequiredExtensions =
                std::ranges::all_of(requiredDeviceExtension,
                    [&availableDeviceExtensions](auto const& requiredDeviceExtension)
                    {
                        return std::ranges::any_of(availableDeviceExtensions,
                            [requiredDeviceExtension](auto const& availableDeviceExtension)
                            { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
                    });

            if (supportsAllRequiredExtensions) {
                score += 1000;
            }

            auto features =
                physicalDevice
                .template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            if (supportsRequiredFeatures) {
                score += 1000;
            }
            

             
            candidates.insert(std::make_pair(score, pd));
        }

        // Check if the best candidate is suitable at all
        if (!candidates.empty() && candidates.rbegin()->first > 0)
        {
            m_physicalDevice = candidates.rbegin()->second;
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice(vk::raii::SurfaceKHR& surface) {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports both graphics and present
        
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
            {
                // found a queue family that supports both graphics and present
                m_queueIndex = qfpIndex;
                break;
            }
        }
        if (m_queueIndex == ~0)
        {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }


        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},                               // vk::PhysicalDeviceFeatures2 (empty for now)
            {.dynamicRendering = true },      // Enable dynamic rendering from Vulkan 1.3
            {.extendedDynamicState = true }   // Enable extended dynamic state from the extension
        };

        float queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = m_queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };

        vk::PhysicalDeviceFeatures deviceFeatures{};

        // Create a chain of feature structures
        

        std::vector<const char*> requiredDeviceExtension = {
            vk::KHRSwapchainExtensionName };

        vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data()
        };
        m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);
    }


}
