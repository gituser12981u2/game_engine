#pragma once

#include <utility>
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

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
    m_memory = std::exchange(other.m_memory, VK_NULL_HANDLE);
    m_size = std::exchange(other.m_size, 0);

    return *this;
  }

  bool init(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size,
            VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps);
  void shutdown() noexcept;

  bool upload(const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

  [[nodiscard]] VkBuffer handle() const noexcept { return m_buffer; }
  [[nodiscard]] VkDeviceSize size() const noexcept { return m_size; }
  [[nodiscard]] bool valid() const noexcept {
    return m_buffer != VK_NULL_HANDLE;
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;       // no-owning
  VkBuffer m_buffer = VK_NULL_HANDLE;       // owning
  VkDeviceMemory m_memory = VK_NULL_HANDLE; // owning
  VkDeviceSize m_size = 0;
};
