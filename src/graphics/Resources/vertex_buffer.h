#pragma once
#include <glm/glm.hpp>
 

#include"vma/vma_buffer.h"
 
#include"../Renderer/Device.h"
#include"vertex.h"

namespace CRATER::ResourceManager {
	class VulkanVertexBuffer {
	private:
	
		 
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Renderer::VulkanDevice& device);

	public:
		VulkanVertexBuffer() = default;
		~VulkanVertexBuffer() = default;

		std::vector<Vertex> vertices;

		vk::VertexInputBindingDescription getBindingDescription()
		{
			return { .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
		}

		std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
		{
			return { {	{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,		.offset = offsetof(Vertex, pos)},
						{.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,	.offset = offsetof(Vertex,color)},
						{.location = 2,	.binding = 0, .format = vk::Format::eR32G32Sfloat,		.offset = offsetof(Vertex,texCoord)}}};
		}

		void createVertexBuffer(const std::vector<Vertex>& vertices,VmaAllocator allocator,Renderer::VulkanDevice& device);

		VkBuffer get() {
			return m_vertexBuffer.buffer();
		}

		auto getVerticeSize() {
			return vertices.size();
		}

		 
		VmaBuffer m_vertexBuffer;
	};
}