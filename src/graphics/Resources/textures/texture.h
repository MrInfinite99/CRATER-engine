#pragma once

 
#include "../vma/vma_buffer.h" 
#include "../../Renderer/Device.h"  
#include"../../Renderer/swap_chain.h"
#include<stb_image.h>

namespace CRATER::ResourceManager {

    class Texture {
    public:
        Texture() = default;

        void createTexture(const char* texture_path,
            VmaAllocator allocator,
            Renderer::VulkanDevice& device) {
            createTextureImage(texture_path,
               allocator,
               device);
            createTextureImageView(device);
            createTextureSampler(device);
        }

        void createTextureImage(const char* texture_path,
            VmaAllocator allocator,
            Renderer::VulkanDevice& device);

        void createTextureImageView(Renderer::VulkanDevice& device);
        void createTextureSampler(Renderer::VulkanDevice& device);
        void  createDepthResources(Renderer::VulkanDevice& device, vk::Extent2D& swapChainExtent);
        vk::Format  findDepthFormat(Renderer::VulkanDevice& device);
        vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, Renderer::VulkanDevice& device);

        auto& view() {
            return textureImageView;
        }

        auto& sampler() {
            return textureSampler;
        }

        vk::raii::Image& depthimage() {
            return depthImage;
        }

        auto& depthimageview() {
            return depthImageView;
        }

    private:
        vk::raii::Image depthImage = nullptr;
        vk::raii::DeviceMemory depthImageMemory = nullptr;
        vk::raii::ImageView depthImageView = nullptr;


        vk::raii::Image textureImage = nullptr;
        vk::raii::DeviceMemory textureImageMemory = nullptr;
        vk::raii::ImageView textureImageView = nullptr;
        vk::raii::Sampler textureSampler = nullptr;
        VmaBuffer m_textureBuffer;


        uint32_t mipLevels{ 0 };
        // Helper Methods
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
            vk::Format format, vk::ImageTiling tiling,
            vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
            vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory,
            Renderer::VulkanDevice& device);

        vk::raii::CommandBuffer beginSingleTimeCommands(Renderer::VulkanDevice& device);

        void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer,
            Renderer::VulkanDevice& device);

        void transitionImageLayout(const vk::raii::Image& image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout, uint32_t mipLevels,
            Renderer::VulkanDevice& device);

        void copyBufferToImage(const VmaBuffer& buffer,
            vk::raii::Image& image,
            uint32_t width, uint32_t height,
            Renderer::VulkanDevice& device);

        uint32_t findMemoryType(uint32_t typeFilter,
            vk::MemoryPropertyFlags properties,
            Renderer::VulkanDevice& device);

        vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, Renderer::VulkanDevice& device);

        
        bool  hasStencilComponent(vk::Format format);
        void generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, Renderer::VulkanDevice& device);
       
        
    };
}