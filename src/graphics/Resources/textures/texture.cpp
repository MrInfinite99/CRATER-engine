#include "Texture.h"
 

namespace CRATER::ResourceManager {

    void Texture::createTextureImage(const char* texture_path, VmaAllocator allocator, Renderer::VulkanDevice& device) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        mipLevels=static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        // Staging Buffer Setup
        VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = imageSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        m_textureBuffer = VmaBuffer(allocator, bufferInfo, allocInfo);

        // Copy pixel data to staging buffer
        memcpy(m_textureBuffer.mappedData(), pixels, imageSize);
        stbi_image_free(pixels);

        // Create the actual Image (using class members)
        createImage(texWidth, texHeight, mipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory, device);

        // Transition and Copy
        transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,mipLevels, device);
        copyBufferToImage(m_textureBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), device);
        //transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,mipLevels, device);
        generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels,device);
    }

    void Texture::createImage(uint32_t width, uint32_t height, uint32_t mipLevels,vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
        vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory, Renderer::VulkanDevice& device) {

        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = format;
        imageInfo.extent = vk::Extent3D{ width, height, 1 }; imageInfo.mipLevels = 1; imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1; imageInfo.tiling = tiling;
        imageInfo.usage = usage; imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.mipLevels = mipLevels;

        image = vk::raii::Image(device.logicalDevice(), imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device)
        };

        imageMemory = vk::raii::DeviceMemory(device.logicalDevice(), allocInfo);
        image.bindMemory(*imageMemory, 0);
    }

    vk::raii::CommandBuffer Texture::beginSingleTimeCommands(Renderer::VulkanDevice& device) {
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

    void Texture::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, Renderer::VulkanDevice& device) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{ };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffer;
    
        device.queue()->submit(submitInfo, nullptr);
        device.queue()->waitIdle();
    }

    void Texture::transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels, Renderer::VulkanDevice& device) {
        auto commandBuffer = beginSingleTimeCommands(device);

        vk::ImageMemoryBarrier barrier{
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .image = *image,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
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

    void Texture::copyBufferToImage(const VmaBuffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, Renderer::VulkanDevice& device) {
        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

        vk::BufferImageCopy region{
            .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
            .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
            .imageOffset = {0, 0, 0}, .imageExtent = {width, height, 1}
        };

        commandBuffer.copyBufferToImage(buffer.buffer(), *image, vk::ImageLayout::eTransferDstOptimal, { region });
        endSingleTimeCommands(commandBuffer, device);
    }

    uint32_t Texture::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, Renderer::VulkanDevice& device) {
        vk::PhysicalDeviceMemoryProperties memProperties = device.physicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Texture::createTextureImageView(Renderer::VulkanDevice& device) {
        textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor,mipLevels,device);
    }


   [[nodiscard]] vk::raii::ImageView Texture::createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, Renderer::VulkanDevice& device) {
        vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
            .format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
        viewInfo.subresourceRange.levelCount = mipLevels;
        return vk::raii::ImageView(device.logicalDevice(), viewInfo);
    }

    void Texture::createTextureSampler(Renderer::VulkanDevice& device) {
        
        vk::PhysicalDeviceProperties properties = device.physicalDevice().getProperties();
        vk::SamplerCreateInfo samplerInfo{
         .magFilter = vk::Filter::eLinear,
         .minFilter = vk::Filter::eLinear,
         .mipmapMode = vk::SamplerMipmapMode::eLinear,
         .addressModeU = vk::SamplerAddressMode::eRepeat,
         .addressModeV = vk::SamplerAddressMode::eRepeat,
         .addressModeW = vk::SamplerAddressMode::eRepeat,
         .mipLodBias = 0.0f,
         .anisotropyEnable = vk::True,
         .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
         .compareEnable = vk::False,
         .compareOp = vk::CompareOp::eAlways,
         .minLod = 0.0f,
         .maxLod = vk::LodClampNone
        };

        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = vk::False;
        samplerInfo.compareEnable = vk::False;
        samplerInfo.compareOp = vk::CompareOp::eAlways;

        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        textureSampler = vk::raii::Sampler(device.logicalDevice(), samplerInfo);
    }

    vk::Format Texture::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, Renderer::VulkanDevice& device) {
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

    vk::Format Texture::findDepthFormat(Renderer::VulkanDevice& device) {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment,
            device
        );
    }

    bool Texture::hasStencilComponent(vk::Format format) {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    void Texture::createDepthResources(Renderer::VulkanDevice& device, vk::Extent2D& swapChainExtent) {
        vk::Format depthFormat = findDepthFormat(device);
        createImage(swapChainExtent.width, swapChainExtent.height,1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory, device);
        depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth,1,device);

    }

    void Texture::generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, Renderer::VulkanDevice& device) {
        

        vk::FormatProperties formatProperties = device.physicalDevice().getFormatProperties(imageFormat);

        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

        vk::ImageMemoryBarrier barrier = { .srcAccessMask = vk::AccessFlagBits::eTransferWrite, .dstAccessMask = vk::AccessFlagBits::eTransferRead, .oldLayout = vk::ImageLayout::eTransferDstOptimal, .newLayout = vk::ImageLayout::eTransferSrcOptimal, .srcQueueFamilyIndex = vk::QueueFamilyIgnored, .dstQueueFamilyIndex = vk::QueueFamilyIgnored, .image = image };
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

            vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dstOffsets;
            offsets[0] = vk::Offset3D(0, 0, 0);
            offsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
            dstOffsets[0] = vk::Offset3D(0, 0, 0);
            dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
            vk::ImageBlit blit = { .srcSubresource = {}, .srcOffsets = offsets, .dstSubresource = {}, .dstOffsets = dstOffsets };
            blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
            blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);

            commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

 
        endSingleTimeCommands(commandBuffer,device);
    }
}  