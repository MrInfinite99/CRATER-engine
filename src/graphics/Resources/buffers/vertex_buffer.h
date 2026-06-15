#pragma once
#include <glm/glm.hpp>
 

#include"../vma/vma_buffer.h"
 
#include"../../Renderer/Device.h"
#include"../vertex.h"

namespace CRATER::Resource {
	class VulkanVertexBuffer {
	private:
	
		 
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Renderer::VulkanDevice& device);

	public:
		VulkanVertexBuffer() = default;
		~VulkanVertexBuffer() = default;

		std::vector<Vertex> vertices;
		std::vector<SkyboxVertex> skyvertices;


		 

		void createVertexBuffer(const std::vector<Vertex>& vertices,VmaAllocator allocator,Renderer::VulkanDevice& device);
		void createVertexBuffer(const std::vector<SkyboxVertex>& m_vertices, VmaAllocator allocator, Renderer::VulkanDevice& device);

		VkBuffer get() {
			return m_vertexBuffer.buffer();
		}

		auto getVerticeSize() {
			return vertices.size();
		}

		 
		VmaBuffer m_vertexBuffer;
	};

	std::vector<vk::VertexInputBindingDescription> getBindingDescription();
	std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();

	std::vector<vk::VertexInputBindingDescription> getSkyboxBindingDescription();
	std::vector<vk::VertexInputAttributeDescription> getSkyboxAttributeDescriptions();
	 
}