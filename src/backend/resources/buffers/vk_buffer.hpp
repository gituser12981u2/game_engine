#pragma once

#include <cstdint>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkBufferObj {
public:
  VkBufferObj() = default;
  ~VkBufferObj() noexcept { shutdown(); }

  VkBufferObj(const VkBufferObj &) = delete;
  VkBufferObj &operator=(const VkBufferObj &) = delete;

  VkBufferObj(VkBufferObj &&other) noexcept { *this = std::move(other); }
  VkBufferObj &operator=(VkBufferObj &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_allocator = std::exchange(other.m_allocator, nullptr);
    m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
    m_allocation = std::exchange(other.m_allocation, nullptr);
    m_size = std::exchange(other.m_size, 0);

    return *this;
  }

  enum class MemUsage : std::uint8_t { GpuOnly, CpuToGpu, GpuToCpu };

  bool init(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
            MemUsage memUsage, bool mapped = false);
  void shutdown() noexcept;

  bool upload(const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

  [[nodiscard]] VkBuffer handle() const noexcept { return m_buffer; }
  [[nodiscard]] VkDeviceSize size() const noexcept { return m_size; }
  [[nodiscard]] bool valid() const noexcept {
    return m_buffer != VK_NULL_HANDLE;
  }

private:
  VmaAllocator m_allocator = nullptr;       // non-owning
  VkBuffer m_buffer = VK_NULL_HANDLE;       // owning
  VkDeviceMemory m_memory = VK_NULL_HANDLE; // owning
  VmaAllocation m_allocation = nullptr;
  VkDeviceSize m_size = 0;
};
