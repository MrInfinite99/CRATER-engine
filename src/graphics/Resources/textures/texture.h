#pragma once
#include"texture_utilities.h"




 

namespace CRATER::Resource {

	struct TextureData {
		ktxTexture2* ktx = nullptr;  // owner – destroy in doUnload()
		uint32_t     width = 0;
		uint32_t     height = 0;
		uint32_t     levels = 1;        // mip levels
		uint32_t     layers = 1;        // array layers / cubemap faces
		vk::Format    format = vk::Format::eUndefined;
		bool         needsTranscode = false;  // true when supercompressed (BasisLZ/ETC1S/UASTC)
		std::string  name;                    // texture URI or generated label
	};

	class Texture:public Resource {
	public:
		Texture() = default;
		explicit Texture(const std::string& id,
			TextureData& td,
			VmaAllocator allocator,
			Renderer::VulkanDevice* device)
			:Resource(id),
			m_td(td),
			m_allocator(allocator),
			m_device(device)

		{}

		~Texture() override {
			doUnload();
		}
		bool doLoad() override {
			createTexture();

			return true;
		}

		void doUnload() override {

		}

		void createTexture();

		 

		void createTextureImageView(Renderer::VulkanDevice& device);
		void createTextureSampler(Renderer::VulkanDevice& device);
		 
 
		void copyBufferToImage(const VmaBuffer& buffer,
			vk::raii::Image& image,
			uint32_t width, uint32_t height,
			Renderer::VulkanDevice& device);

		void generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount, Renderer::VulkanDevice& device);
 
		vk::raii::ImageView& view() {
			return textureImageView;
		}

		vk::raii::Sampler& sampler() {
			return textureSampler;
		}

		

	private:
		


		vk::raii::Image textureImage = nullptr;
		vk::raii::DeviceMemory textureImageMemory = nullptr;
		vk::raii::ImageView textureImageView = nullptr;
		vk::raii::Sampler textureSampler = nullptr;
		VmaBuffer m_textureBuffer;
		Renderer::VulkanDevice* m_device{};
		TextureData m_td;
		VmaAllocator m_allocator;


		uint32_t mipLevels{ 0 };
		 

		vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, Renderer::VulkanDevice& device);


		bool  hasStencilComponent(vk::Format format);

		

		void uploadKtxMipLevels(ktxTexture2* ktx,
			vk::Format                vkFmt,
			uint32_t                width,
			uint32_t                height,
			uint32_t                levels,
			VmaAllocator            allocator,
			Renderer::VulkanDevice& device);



	};

	class DepthTexture {
		private:
			vk::raii::Image depthImage = nullptr;
			vk::raii::DeviceMemory depthImageMemory = nullptr;
			vk::raii::ImageView depthImageView = nullptr;
			vk::Format depthFormat;

			vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, Renderer::VulkanDevice& device) {
				vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
					.format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
				viewInfo.subresourceRange.levelCount = mipLevels;
				return vk::raii::ImageView(device.logicalDevice(), viewInfo);
			}

	public:

		vk::Format createDepthResources(Renderer::VulkanDevice& device, vk::Extent2D& swapChainExtent) {
			depthFormat = findDepthFormat(device);
			createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, {},1, depthImage, depthImageMemory, device);
			depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1, device);

			return depthFormat;
		}

		const vk::Format getDepthFormat() {
			return depthFormat;
		}

		vk::raii::Image& depthimage() {
			return depthImage;
		}

		auto& depthimageview() {
			return depthImageView;
		}
	};
 

	class SkyboxTexture :public Resource {
	public:
		SkyboxTexture() = default;
		explicit SkyboxTexture(const std::string& id,
			const std::string& ktxPath,
			VmaAllocator allocator,
			Renderer::VulkanDevice* device)
			: Resource(id) ,
			m_path(ktxPath),
			m_allocator(allocator),
			m_device(device)

		{}
		~SkyboxTexture() override { doUnload(); }

		void createSkybox();

		vk::raii::ImageView& view()  { return m_imageView; }
		vk::raii::Sampler& sampler() { return m_sampler; }

		//void doUnload() override;
		bool doLoad() override {
			createSkybox();
			return true;
		}

		void doUnload() override {
			return;
		}

	private:
		uint32_t               m_mipLevels{ 0 };
		VmaBuffer              m_stagingBuffer;

		vk::raii::Image        m_image = nullptr;
		vk::raii::DeviceMemory m_imageMemory = nullptr;
		vk::raii::ImageView    m_imageView = nullptr;
		vk::raii::Sampler      m_sampler = nullptr;
		const std::string m_path;
		VmaAllocator m_allocator;
		Renderer::VulkanDevice* m_device{};

		void createCubemapImage(ktxTexture2* ktx,
			vk::Format                vkFmt,
			VmaAllocator            allocator,
			Renderer::VulkanDevice& device);

		void createSkyboxImageView(vk::Format vkFmt,
			Renderer::VulkanDevice& device);

		void createSkyboxSampler(Renderer::VulkanDevice& device);

		// Override createImage to set eCubeCompatible + 6 array layers
		 
	};

	 
}