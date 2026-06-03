#pragma once
#include<vma/vk_mem_alloc.h>
#include<stdexcept>
#include<utility>

namespace CRATER::Resource{
    class VmaBuffer {
    public:
        VmaBuffer() = default;

        VmaBuffer(VmaAllocator allocator, const VkBufferCreateInfo& bufferInfo,
            const VmaAllocationCreateInfo& allocInfo)
            : m_allocator(allocator) {
            VkResult result = vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
                &m_buffer, &m_allocation, &m_allocInfo);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create VMA buffer");
            }
        }

        ~VmaBuffer() {
            if (m_buffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
            }
        }

        // Delete copy, allow move
        VmaBuffer(const VmaBuffer&) = delete;
        VmaBuffer& operator=(const VmaBuffer&) = delete;

        VmaBuffer(VmaBuffer&& other) noexcept
            : m_allocator(other.m_allocator)
            , m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE))
            , m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE))
            , m_allocInfo(other.m_allocInfo) {
        }

        VmaBuffer& operator=(VmaBuffer&& other) noexcept {
            if (this != &other) {
                // Destroy current resource
                if (m_buffer != VK_NULL_HANDLE) {
                    vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
                }

                // Take ownership
                m_allocator = other.m_allocator;
                m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
                m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
                m_allocInfo = other.m_allocInfo;
            }
            return *this;
        }

        // Accessors
        VkBuffer buffer() const { return m_buffer; }
        VmaAllocation allocation() const { return m_allocation; }
        void* mappedData() const { return m_allocInfo.pMappedData; }

        // Implicit conversion for convenience
        operator VkBuffer() const { return m_buffer; }

    private:
        VmaAllocator m_allocator = VK_NULL_HANDLE;
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocInfo = {};
    };
}