#pragma once

#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkImageObj {
public:
  VkImageObj() = default;
  ~VkImageObj() noexcept { shutdown(); }

  VkImageObj(const VkImageObj &) = delete;
  VkImageObj &operator=(const VkImageObj &) = delete;

  VkImageObj(VkImageObj &&other) noexcept { *this = std::move(other); }
  VkImageObj &operator=(VkImageObj &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_allocator = std::exchange(other.m_allocator, nullptr);
    m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
    m_allocation = std::exchange(other.m_allocation, nullptr);
    m_format = std::exchange(other.m_format, VK_FORMAT_UNDEFINED);
    m_width = std::exchange(other.m_width, 0U);
    m_height = std::exchange(other.m_height, 0U);

    return *this;
  }

  bool init2D(VmaAllocator allocator, uint32_t width, uint32_t height,
              VkFormat format, VkImageUsageFlags usage,
              VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
  void shutdown() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return m_image != VK_NULL_HANDLE;
  }
  [[nodiscard]] VkImage handle() const noexcept { return m_image; }
  [[nodiscard]] VkFormat format() const noexcept { return m_format; }
  [[nodiscard]] uint32_t width() const noexcept { return m_width; }
  [[nodiscard]] uint32_t height() const noexcept { return m_height; }

private:
  VmaAllocator m_allocator = nullptr;   // non-owning
  VkImage m_image = VK_NULL_HANDLE;     // owning
  VmaAllocation m_allocation = nullptr; // owning
  VkFormat m_format = VK_FORMAT_UNDEFINED;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
};
