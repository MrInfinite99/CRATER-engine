#include "Texture.h"
 

namespace CRATER::Resource {

    void Texture::createTexture()
       
    {
        if (!m_td.ktx)
            throw std::runtime_error("[Texture] TextureData has null ktxTexture2*");

        vk::Format uploadFormat = static_cast<vk::Format>(m_td.format); 

        // Transcode BasisLZ / UASTC supercompression if needed
        if (m_td.needsTranscode) {
            const ktx_transcode_fmt_e target = selectTranscodeFormat(*m_device);

            const KTX_error_code res =
                ktxTexture2_TranscodeBasis(m_td.ktx, target,0);

            if (res != KTX_SUCCESS)
                throw std::runtime_error(
                    std::string("[Texture] TranscodeBasis failed: ") + ktxErrorString(res));

            // libktx updates vkFormat in-place after transcoding
            uploadFormat = static_cast<vk::Format>(m_td.ktx->vkFormat);
        }

        uploadKtxMipLevels(m_td.ktx, uploadFormat,
            m_td.width, m_td.height, m_td.levels,
            m_allocator, *m_device);

        createTextureImageView(*m_device);
        createTextureSampler(*m_device);
    }
 
    void Texture::uploadKtxMipLevels(ktxTexture2* ktx,
        vk::Format                vkFmt,
        uint32_t                width,
        uint32_t                height,
        uint32_t                levels,
        VmaAllocator            allocator,
        Renderer::VulkanDevice& device)
    {
        mipLevels = levels; // store so createTextureImageView picks up the right count

        // ── 1. Collect per-level offsets and build copy regions ───────────────
        std::vector<vk::BufferImageCopy> regions;
        regions.reserve(levels);

        for (uint32_t lvl = 0; lvl < levels; ++lvl) {
            ktx_size_t offset = 0;
            if (ktxTexture_GetImageOffset(ktxTexture(ktx), lvl, 0, 0, &offset) != KTX_SUCCESS)
                throw std::runtime_error("[Texture] ktxTexture_GetImageOffset failed");

            vk::BufferImageCopy r;
            r.bufferOffset = static_cast<vk::DeviceSize>(offset);
            r.bufferRowLength = 0;
            r.bufferImageHeight = 0;
            r.imageSubresource = { vk::ImageAspectFlagBits::eColor, lvl, 0, 1 };
            r.imageOffset = { 0, 0, 0 };
            r.imageExtent = { std::max(1u, width >> lvl),
                                    std::max(1u, height >> lvl), 1 };
            regions.push_back(r);
        }

        // ── 2. Single staging buffer for all mip data ─────────────────────────
        const size_t totalBytes = ktxTexture_GetDataSize(ktxTexture(ktx));

        VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufInfo.size = totalBytes;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo vmaInfo{};
        vmaInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        vmaInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaBuffer staging;
        staging = VmaBuffer(allocator,bufInfo,vmaInfo);
        

        memcpy(staging.mappedData(),
            ktxTexture_GetData(ktxTexture(ktx)),
            totalBytes);

        // ── 3. Create VkImage ─────────────────────────────────────────────────
        createImage(width, height, levels,
            static_cast<vk::Format>(vkFmt),
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,{},1,
            textureImage, textureImageMemory, device);

        // ── 4. Undefined → TransferDst ────────────────────────────────────────
        transitionImageLayout(textureImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            levels, 1, device);

        // ── 5. Copy all mips in one command buffer ────────────────────────────
        auto cmd = beginSingleTimeCommands(device);
        cmd.copyBufferToImage(staging.buffer(), *textureImage,
            vk::ImageLayout::eTransferDstOptimal, regions);
        endSingleTimeCommands(cmd, device);

        // ── 6. TransferDst → ShaderReadOnly ──────────────────────────────────
        transitionImageLayout(textureImage,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            levels, 1, device);

         
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

    bool Texture::hasStencilComponent(vk::Format format) {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    void Texture::generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels,uint32_t layerCount, Renderer::VulkanDevice& device) {
        

        vk::FormatProperties formatProperties = device.physicalDevice().getFormatProperties(imageFormat);

        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

        vk::ImageMemoryBarrier barrier = { .srcAccessMask = vk::AccessFlagBits::eTransferWrite, .dstAccessMask = vk::AccessFlagBits::eTransferRead, .oldLayout = vk::ImageLayout::eTransferDstOptimal, .newLayout = vk::ImageLayout::eTransferSrcOptimal, .srcQueueFamilyIndex = vk::QueueFamilyIgnored, .dstQueueFamilyIndex = vk::QueueFamilyIgnored, .image = image };
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;
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
            blit.srcSubresource.layerCount = layerCount;
            blit.dstSubresource.layerCount = layerCount;

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

namespace CRATER::Resource {
    void SkyboxTexture::createSkybox()
    {
        // ── 1. Load KTX2 file ─────────────────────────────────────────────────
        ktxTexture2* ktx = nullptr;
        KTX_error_code res = ktxTexture2_CreateFromNamedFile(
            m_path.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &ktx);

        if (res != KTX_SUCCESS)
            throw std::runtime_error(
                std::string("[SkyboxTexture] Failed to load KTX2 file: ") + ktxErrorString(res));

        // Sanity check — must be a cubemap (6 faces)
        if (ktx->numFaces != 6)
            throw std::runtime_error("[SkyboxTexture] KTX2 file is not a cubemap (numFaces != 6)");

        std::cout << "[SkyboxTexture] KTX2 info:\n"
            << "  path        : " << m_path << "\n"
            << "  vkFormat    : " << ktx->vkFormat << "\n";

        // ── 2. Transcode if supercompressed ───────────────────────────────────
        vk::Format uploadFormat = static_cast<vk::Format>(ktx->vkFormat);

        if (ktxTexture2_NeedsTranscoding(ktx)) {
            const ktx_transcode_fmt_e target = selectTranscodeFormat(*m_device);

            res = ktxTexture2_TranscodeBasis(ktx, target,0);
            if (res != KTX_SUCCESS) {
                ktxTexture_Destroy(ktxTexture(ktx));
                throw std::runtime_error(
                    std::string("[SkyboxTexture] TranscodeBasis failed: ") + ktxErrorString(res));
            }

            uploadFormat = static_cast<vk::Format>(ktx->vkFormat);
        }

        // ── 3. Upload, create view + sampler ──────────────────────────────────
        try {
            createCubemapImage(ktx, uploadFormat, m_allocator, *m_device);
            createSkyboxImageView(uploadFormat, *m_device);
            createSkyboxSampler(*m_device);
        }
        catch (...) {
            ktxTexture_Destroy(ktxTexture(ktx));
            throw;
        }

        ktxTexture_Destroy(ktxTexture(ktx));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // GPU upload — all 6 faces × all mip levels in one staging buffer
    // ─────────────────────────────────────────────────────────────────────────────

    void SkyboxTexture::createCubemapImage(ktxTexture2* ktx,
        vk::Format                vkFmt,
        VmaAllocator            allocator,
        Renderer::VulkanDevice& device)
    {
        const uint32_t width = ktx->baseWidth;
        const uint32_t height = ktx->baseHeight;
        m_mipLevels = ktx->numLevels;

        // ── a. Build one copy region per face per mip ─────────────────────────
        // KTX2 memory layout: mip-major, then face, then layer.
        // ktxTexture_GetImageOffset(ktx, level, layer=0, face) gives the offset.
        std::vector<vk::BufferImageCopy> regions;
        regions.reserve(m_mipLevels * 6);

        for (uint32_t lvl = 0; lvl < m_mipLevels; ++lvl) {
            for (uint32_t face = 0; face < 6; ++face) {
                ktx_size_t offset = 0;
                // In libktx the face index is the 3rd param for cube arrays
                KTX_error_code res = ktxTexture_GetImageOffset(
                    ktxTexture(ktx), lvl, 0, face, &offset);

                if (res != KTX_SUCCESS)
                    throw std::runtime_error("[SkyboxTexture] ktxTexture_GetImageOffset failed");

                vk::BufferImageCopy region;
                region.bufferOffset = static_cast<vk::DeviceSize>(offset);
                region.bufferRowLength = 0;   // tightly packed
                region.bufferImageHeight = 0;
                region.imageSubresource = {
                    vk::ImageAspectFlagBits::eColor,
                    lvl,    // mip level
                    face,   // baseArrayLayer = face index for cubemaps
                    1       // layerCount
                };
                region.imageOffset = { 0, 0, 0 };
                region.imageExtent = {
                    std::max(1u, width >> lvl),
                    std::max(1u, height >> lvl),
                    1
                };
                regions.push_back(region);
            }
        }

        // ── b. Single staging buffer for all face + mip data ──────────────────
        const size_t totalBytes = ktxTexture_GetDataSize(ktxTexture(ktx));

        VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufInfo.size = totalBytes;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo vmaInfo{};
        vmaInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        m_stagingBuffer = VmaBuffer(allocator, bufInfo, vmaInfo);
        memcpy(m_stagingBuffer.mappedData(),
            ktxTexture_GetData(ktxTexture(ktx)),
            totalBytes);

        // ── c. Create cube VkImage ─────────────────────────────────────────────
        createImage(width, height, m_mipLevels,
            static_cast<vk::Format>(vkFmt),
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,vk::ImageCreateFlagBits::eCubeCompatible,6,
            m_image, m_imageMemory, device);

        // ── d. Transition all 6 faces → TransferDst ───────────────────────────
        transitionImageLayout(m_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            m_mipLevels, 6, device);

        // ── e. Copy all regions in one command buffer ──────────────────────────
        auto cmd =beginSingleTimeCommands(device);
        cmd.copyBufferToImage(m_stagingBuffer.buffer(),
            *m_image,
            vk::ImageLayout::eTransferDstOptimal,
            regions);
        endSingleTimeCommands(cmd, device);

        // ── f. Transition → ShaderReadOnly ────────────────────────────────────
        transitionImageLayout(m_image,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            m_mipLevels, 6, device);
    }
 

    void SkyboxTexture::createSkyboxImageView(vk::Format                vkFmt,
        Renderer::VulkanDevice& device)
    {
        vk::ImageViewCreateInfo info{};
        info.image = *m_image;
        info.viewType = vk::ImageViewType::eCube;
        info.format = static_cast<vk::Format>(vkFmt);
        info.subresourceRange = {
            vk::ImageAspectFlagBits::eColor,
            0,            // baseMipLevel
            m_mipLevels,  // levelCount
            0,            // baseArrayLayer
            6             // layerCount — all 6 cube faces
        };

        m_imageView = vk::raii::ImageView(device.logicalDevice(), info);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Sampler — clamp to edge, full mip range
    // ─────────────────────────────────────────────────────────────────────────────

    void SkyboxTexture::createSkyboxSampler(Renderer::VulkanDevice& device)
    {
        const auto props = device.physicalDevice().getProperties();

        vk::SamplerCreateInfo info{};
        info.magFilter = vk::Filter::eLinear;
        info.minFilter = vk::Filter::eLinear;
        info.mipmapMode = vk::SamplerMipmapMode::eLinear;

        // Clamp to edge is correct for cubemaps — avoids seams at face boundaries
        info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        info.addressModeW = vk::SamplerAddressMode::eClampToEdge;

        info.anisotropyEnable = vk::True;
        info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
        info.compareEnable = vk::False;
        info.minLod = 0.0f;
        info.maxLod = static_cast<float>(m_mipLevels);

        m_sampler = vk::raii::Sampler(device.logicalDevice(), info);
    }
}