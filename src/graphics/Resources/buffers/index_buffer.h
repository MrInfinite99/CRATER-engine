#pragma once
 
#include"../vma/vma_buffer.h"
#include"../../Renderer/Device.h"
namespace CRATER::Resource {
	class VulkanIndexBuffer {
	private:
		std::vector<uint32_t> indices;


		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Renderer::VulkanDevice& device);
		

	public:
		void createIndexBuffer(const std::vector<uint32_t>& m_indices,VmaAllocator allocator, Renderer::VulkanDevice& device);

		VkBuffer get() {
			return m_indexBuffer.buffer();
		}

		auto getIndicesSize() {
			return indices.size();
		}

		VmaBuffer m_indexBuffer;
	};
}