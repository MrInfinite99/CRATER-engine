#include "Device.h"
#include <stdexcept>
#include <set>

namespace CRATER::Renderer {

    void VulkanDevice::init(vk::raii::Context& context, vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface) {
        // Select a suitable physical device (first discrete GPU or first available)
        pickPhysicalDevice(instance);
        // Find queue family with graphics support
        createLogicalDevice(surface);

        m_graphicsQueue = vk::raii::Queue(m_device, m_graphicsIndex, 0);
        m_presentQueue = vk::raii::Queue(m_device, m_presentIndex, 0);

        vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    .queueFamilyIndex = m_graphicsIndex };

        m_commandPool = vk::raii::CommandPool(m_device,poolInfo);
    }

    void VulkanDevice::pickPhysicalDevice(vk::raii::Instance& instance) {
        auto physicalDevices = vk::raii::PhysicalDevices(instance);
        if (physicalDevices.empty())
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<int, vk::raii::PhysicalDevice> candidates;

        for (const auto& physicalDevice : physicalDevices)
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
            std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };
           
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
            

             
            candidates.insert(std::make_pair(score, physicalDevice));
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

    static uint32_t findQueueFamilies(vk::raii::PhysicalDevice physicalDevice) {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports graphics
        auto graphicsQueueFamilyProperty =
            std::find_if(queueFamilyProperties.begin(),
                queueFamilyProperties.end(),
                [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });

        return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    }

    void VulkanDevice::createLogicalDevice(vk::raii::SurfaceKHR& surface) {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();
         m_graphicsIndex = findQueueFamilies(m_physicalDevice);
        float queuePriority = 0.5f;
        // determine a queueFamilyIndex that supports present
       // first check if the graphicsIndex is good enough
        m_presentIndex = m_physicalDevice.getSurfaceSupportKHR(m_graphicsIndex, *surface)
            ? m_graphicsIndex
            : static_cast<uint32_t>(queueFamilyProperties.size());
        if (m_presentIndex == queueFamilyProperties.size())
        {
            // the graphicsIndex doesn't support present -> look for another family index that supports both
            // graphics and present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                    m_physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
                {
                    m_graphicsIndex = static_cast<uint32_t>(i);
                    m_presentIndex = m_graphicsIndex;
                    break;
                }
            }
            if (m_presentIndex == queueFamilyProperties.size())
            {
                // there's nothing like a single family index that supports both graphics and present -> look for another
                // family index that supports present
                for (size_t i = 0; i < queueFamilyProperties.size(); i++)
                {
                    if (m_physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
                    {
                        m_presentIndex = static_cast<uint32_t>(i);
                        break;
                    }
                }
            }
        }
        if ((m_graphicsIndex == queueFamilyProperties.size()) || (m_presentIndex == queueFamilyProperties.size()))
        {
            throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
        }

       
        // create a Device

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {.features = {.samplerAnisotropy = true}},                   // vk::PhysicalDeviceFeatures2
            {.synchronization2 = true, .dynamicRendering = true},        // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true}                               // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

        std::vector<const char*> deviceExtensions = {
            vk::KHRSwapchainExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRSynchronization2ExtensionName,
            vk::KHRCreateRenderpass2ExtensionName
        };

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = m_graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
        vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(), .queueCreateInfoCount = 1, .pQueueCreateInfos = &deviceQueueCreateInfo};
        deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);
    }


}
