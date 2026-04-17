#include"vertex_buffer.h"

namespace CRATER::ResourceManager {

	void VulkanVertexBuffer::createVertexBuffer(const std::vector<Vertex>& m_vertices,VmaAllocator allocator, Renderer::VulkanDevice& device) {
        vertices = m_vertices;
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Staging buffer
        VkBufferCreateInfo stagingInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaBuffer stagingBuffer(allocator, stagingInfo, stagingAllocInfo);

        // Copy data (already mapped)
        memcpy(stagingBuffer.mappedData(), vertices.data(), bufferSize);

        // Device-local vertex buffer
        VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

        m_vertexBuffer = VmaBuffer(allocator, bufferInfo, allocInfo);

        // Copy from staging to device-local
        copyBuffer(stagingBuffer.buffer(), m_vertexBuffer.buffer(), bufferSize, device);
		 
	} 

  

	void VulkanVertexBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,Renderer::VulkanDevice& device) {
		vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eTransient,
	.queueFamilyIndex = device.graphicsIndex() };

		auto commandPool = vk::raii::CommandPool(device.logicalDevice(), poolInfo);
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::raii::CommandBuffer commandCopyBuffer = std::move(device.logicalDevice().allocateCommandBuffers(allocInfo).front());

		commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
		commandCopyBuffer.end();

        device.queue()->submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer }, nullptr);
        device.queue()->waitIdle();
	 }
	 

}