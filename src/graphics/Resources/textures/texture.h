#pragma once


#include "../vma/vma_buffer.h" 
#include "../../Renderer/Device.h"  
#include"../../Renderer/swap_chain.h"
#include<stb_image.h>

namespace CRATER::Resource {

	class Texture {
	public:
		Texture() = default;

		virtual void createTexture(const char* texture_path,
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

		uint32_t findMemoryType(uint32_t typeFilter,
			vk::MemoryPropertyFlags properties,
			Renderer::VulkanDevice& device);

		void transitionImageLayout(const vk::raii::Image& image,
			vk::ImageLayout oldLayout,
			vk::ImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount,
			Renderer::VulkanDevice& device);

		void copyBufferToImage(const VmaBuffer& buffer,
			vk::raii::Image& image,
			uint32_t width, uint32_t height,
			Renderer::VulkanDevice& device);

		void generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount, Renderer::VulkanDevice& device);

		vk::raii::CommandBuffer beginSingleTimeCommands(Renderer::VulkanDevice& device);

		void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer,
			Renderer::VulkanDevice& device);

		virtual vk::raii::ImageView& view() {
			return textureImageView;
		}

		virtual vk::raii::Sampler& sampler() {
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




		vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, Renderer::VulkanDevice& device);


		bool  hasStencilComponent(vk::Format format);



	};

	class SkyboxTexture :public Texture {
	private:
		std::vector<stbi_uc*> pixels;
		uint32_t mipLevels{ 0 };
		VmaBuffer m_skyboxBuffer;
		vk::raii::Image skyboxImage = nullptr;
		vk::raii::DeviceMemory skyboxImageMemory = nullptr;
		vk::raii::ImageView skyboxImageView = nullptr;
		vk::raii::Sampler skyboxSampler = nullptr;

	public:
		vk::raii::ImageView& view() override {
			return skyboxImageView;
		}

		vk::raii::Sampler& sampler() override {
			return skyboxSampler;
		}


		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
			vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
			vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory, Renderer::VulkanDevice& device)
		{
			vk::ImageCreateInfo imageInfo{};
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.format = format;
			imageInfo.extent = vk::Extent3D{ width, height, 1 }; imageInfo.mipLevels = 1; imageInfo.arrayLayers = 6;
			imageInfo.samples = vk::SampleCountFlagBits::e1; imageInfo.tiling = tiling;
			imageInfo.usage = usage; imageInfo.sharingMode = vk::SharingMode::eExclusive;
			imageInfo.mipLevels = mipLevels;
			imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

			image = vk::raii::Image(device.logicalDevice(), imageInfo);

			vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
			vk::MemoryAllocateInfo allocInfo{
				.allocationSize = memRequirements.size,
				.memoryTypeIndex = Texture::findMemoryType(memRequirements.memoryTypeBits, properties, device)
			};

			imageMemory = vk::raii::DeviceMemory(device.logicalDevice(), allocInfo);
			image.bindMemory(*imageMemory, 0);
		}

		void copyBufferToImage(const VmaBuffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, Renderer::VulkanDevice& device) {
			vk::raii::CommandBuffer commandBuffer = Texture::beginSingleTimeCommands(device);

			vk::DeviceSize layerSize = width * height * 4;

			std::vector<vk::BufferImageCopy> regions;

			for (uint32_t face = 0; face < 6; ++face)
			{
				regions.push_back(
					vk::BufferImageCopy{
						.bufferOffset = face * layerSize,
						.bufferRowLength = 0,
						.bufferImageHeight = 0,
						.imageSubresource =
						{
							vk::ImageAspectFlagBits::eColor,
							0,          // mip level
							face,       // cubemap face
							1
						},
						.imageOffset = {0,0,0},
						.imageExtent = {width,height,1}
					}
				);
			}

			commandBuffer.copyBufferToImage(buffer.buffer(), *image, vk::ImageLayout::eTransferDstOptimal, regions);
			Texture::endSingleTimeCommands(commandBuffer, device);
		}

		void createSkyboxImage(std::vector<std::string> skyboxPath, VmaAllocator allocator, Renderer::VulkanDevice& device) {
			int texWidth, texHeight, texChannels;
			for (int i = 0; i < 6; i++)
			{

				stbi_uc* data = stbi_load(
					skyboxPath[i].c_str(),
					&texWidth,
					&texHeight,
					&texChannels,
					STBI_rgb_alpha
				);

				if (!data)
				{
					throw std::runtime_error("Failed to load cubemap face: " + skyboxPath[i]);
				}

				pixels.push_back(data);
			}


			mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
			vk::DeviceSize imageSize = texWidth * texHeight * 4 * 6;
			vk::DeviceSize layerSize = imageSize / 6;


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

			m_skyboxBuffer = VmaBuffer(allocator, bufferInfo, allocInfo);

			uint8_t* dst = static_cast<uint8_t*>(m_skyboxBuffer.mappedData());
			// Copy pixel data to staging buffer
			for (int i = 0; i < 6; i++)
			{
				memcpy(
					dst + i * layerSize,
					pixels[i],
					layerSize
				);
			}

			// memcpy(m_skyboxBuffer.mappedData(), pixels.data(), imageSize);
			for (auto p : pixels) {
				stbi_image_free(p);
			}

			createImage(texWidth, texHeight, mipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				vk::MemoryPropertyFlagBits::eDeviceLocal, skyboxImage, skyboxImageMemory, device);

			// Transition and Copy
			Texture::transitionImageLayout(skyboxImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels, 6, device);
			copyBufferToImage(m_skyboxBuffer, skyboxImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), device);

			Texture::generateMipmaps(skyboxImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels, 6, device);
		}

		void createSkyboxImageView(Renderer::VulkanDevice& device) {
			vk::ImageViewCreateInfo viewInfo{ .image = skyboxImage, .viewType = vk::ImageViewType::eCube,
			.format = vk::Format::eR8G8B8A8Srgb, .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6} };//layerCount =6
			viewInfo.subresourceRange.levelCount = mipLevels;
			skyboxImageView = vk::raii::ImageView(device.logicalDevice(), viewInfo);
		}

		void createSkyboxSampler(Renderer::VulkanDevice& device) {
			vk::PhysicalDeviceProperties properties = device.physicalDevice().getProperties();
			vk::SamplerCreateInfo samplerInfo{
   .magFilter = vk::Filter::eLinear,
   .minFilter = vk::Filter::eLinear,

   .mipmapMode = vk::SamplerMipmapMode::eLinear,

   .addressModeU = vk::SamplerAddressMode::eClampToEdge,
   .addressModeV = vk::SamplerAddressMode::eClampToEdge,
   .addressModeW = vk::SamplerAddressMode::eClampToEdge,

   .anisotropyEnable = vk::True,
   .maxAnisotropy = properties.limits.maxSamplerAnisotropy,

   .compareEnable = vk::False,

   .minLod = 0.0f,

   .maxLod = static_cast<float>(mipLevels)
			};

			skyboxSampler = vk::raii::Sampler(device.logicalDevice(), samplerInfo);
		}

		void createSkybox(std::vector<std::string> skyboxPath, VmaAllocator allocator, Renderer::VulkanDevice& device) {
			createSkyboxImage(skyboxPath, allocator, device);

			createSkyboxImageView(device);
			createSkyboxSampler(device);
		}
	};
}