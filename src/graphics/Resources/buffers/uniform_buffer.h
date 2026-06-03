#pragma once
#include"../vma/vma_buffer.h"
#include"../../Renderer/Device.h"
#include"../../constants.h"
#include<vector>
#include <chrono>

namespace CRATER::Resource {

	template<typename T>
	class VulkanUniformBuffer {
	public:
		void createUniformBuffer(VmaAllocator allocator, Renderer::VulkanDevice& device);

		void updateUniformBuffer(uint32_t currentImage, const T& data);

        auto& operator[] (size_t i) {
            return m_uniformBuffer[i];
        }
	private:
		 
		std::vector<VmaBuffer> m_uniformBuffer;
		 
	};
}

namespace CRATER::Resource {
    template<typename T>
    void VulkanUniformBuffer<T>::createUniformBuffer(VmaAllocator allocator, Renderer::VulkanDevice& device) {
        m_uniformBuffer.clear();
        vk::DeviceSize bufferSize = sizeof(T);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;

            
            m_uniformBuffer.emplace_back(allocator, bufferInfo, allocInfo);
        }
    }

    template<typename T>
    void VulkanUniformBuffer<T>::updateUniformBuffer(uint32_t currentImage, const T& data) {
        memcpy(m_uniformBuffer[currentImage].mappedData(), &data, sizeof(T));


    }
}