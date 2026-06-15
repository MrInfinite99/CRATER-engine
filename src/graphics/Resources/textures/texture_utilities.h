#pragma once
#include "../vma/vma_buffer.h" 
#include "../../Renderer/Device.h"  
#include"../../Renderer/swap_chain.h"
#include"../resource_manager.h"
#include<ktx.h>
 
namespace CRATER::Resource {



     
    inline uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, Renderer::VulkanDevice& device) {
        vk::PhysicalDeviceMemoryProperties memProperties = device.physicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }


    inline void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::ImageCreateFlagBits flags, uint32_t arrayLayers,
        vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory, Renderer::VulkanDevice& device) {

        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = format;
        imageInfo.extent = vk::Extent3D{ width, height, 1 }; 
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = tiling;
        imageInfo.usage = usage; 
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.flags = flags;
        

        image = vk::raii::Image(device.logicalDevice(), imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device)
        };

        imageMemory = vk::raii::DeviceMemory(device.logicalDevice(), allocInfo);
        image.bindMemory(*imageMemory, 0);
    }

    inline vk::raii::CommandBuffer beginSingleTimeCommands(Renderer::VulkanDevice& device) {
        // Note: In a real engine, you should use a persistent Command Pool
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device.graphicsIndex()
        };

        // For single-time commands, we'll use the device's command pool logic
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = *device.commandPool(), // Assuming your device wrapper owns a pool
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffer commandBuffer = std::move(device.logicalDevice().allocateCommandBuffers(allocInfo).front());
        commandBuffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        return commandBuffer;
    }

    inline void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, Renderer::VulkanDevice& device) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{ };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffer;

        device.queue()->submit(submitInfo, nullptr);
        device.queue()->waitIdle();
    }

    inline void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount, Renderer::VulkanDevice& device) {
        auto commandBuffer = beginSingleTimeCommands(device);

        vk::ImageMemoryBarrier barrier{
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .image = *image,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, layerCount }
        };
        barrier.subresourceRange.levelCount = mipLevels;

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
        endSingleTimeCommands(commandBuffer, device);
    }

    inline [[nodiscard]] vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, Renderer::VulkanDevice& device) {
        vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
            .format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
        viewInfo.subresourceRange.levelCount = mipLevels;
        return vk::raii::ImageView(device.logicalDevice(), viewInfo);
    }

    inline ktx_transcode_fmt_e selectTranscodeFormat(Renderer::VulkanDevice& device)
    {
        const auto features = device.physicalDevice().getFeatures(); // adjust to your accessor

        if (features.textureCompressionBC)
            return KTX_TTF_BC7_RGBA;        // desktop: best quality

        if (features.textureCompressionASTC_LDR)
            return KTX_TTF_ASTC_4x4_RGBA;  // Mali / Apple Silicon

        if (features.textureCompressionETC2)
            return KTX_TTF_ETC2_RGBA;       // universal Android fallback

        std::cerr << "[Texture] No compressed format supported, falling back to RGBA32\n";
        return KTX_TTF_RGBA32;
    }
 
    inline vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, Renderer::VulkanDevice& device) {
        for (const auto format : candidates) {
            vk::FormatProperties props = device.physicalDevice().getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    inline vk::Format findDepthFormat(Renderer::VulkanDevice& device) {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment,
            device
        );
    }

    

    

    
}

