#pragma once
#include<vma/vk_mem_alloc.h>

class VmaAllocatorRAII {
public:
    VmaAllocatorRAII() = default;

    VmaAllocatorRAII(const VmaAllocatorCreateInfo& createInfo) {
        VkResult result = vmaCreateAllocator(&createInfo, &m_allocator);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator");
        }
    }

    ~VmaAllocatorRAII() {
        if (m_allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(m_allocator);
        }
    }

    // Delete copy, allow move
    VmaAllocatorRAII(const VmaAllocatorRAII&) = delete;
    VmaAllocatorRAII& operator=(const VmaAllocatorRAII&) = delete;

    VmaAllocatorRAII(VmaAllocatorRAII&& other) noexcept
        : m_allocator(std::exchange(other.m_allocator, VK_NULL_HANDLE)) {
    }

    VmaAllocatorRAII& operator=(VmaAllocatorRAII&& other) noexcept {
        if (this != &other) {
            if (m_allocator != VK_NULL_HANDLE) {
                vmaDestroyAllocator(m_allocator);
            }
            m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);
        }
        return *this;
    }

    operator VmaAllocator() const { return m_allocator; }
    VmaAllocator get() const { return m_allocator; }

private:
    VmaAllocator m_allocator = VK_NULL_HANDLE;
};