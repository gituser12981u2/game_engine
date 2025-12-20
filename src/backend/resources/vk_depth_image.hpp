#pragma once

#include <utility>
#include <vulkan/vulkan_core.h>

class VkDepthImage {
public:
  VkDepthImage() = default;
  ~VkDepthImage() noexcept { shutdown(); }

  VkDepthImage(const VkDepthImage &) = delete;
  VkDepthImage &operator=(const VkDepthImage &) = delete;

  VkDepthImage(VkDepthImage &&other) noexcept { *this = std::move(other); }
  VkDepthImage &operator=(VkDepthImage &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
    m_mem = std::exchange(other.m_mem, VK_NULL_HANDLE);
    m_view = std::exchange(other.m_view, VK_NULL_HANDLE);
    m_format = std::exchange(other.m_format, VK_FORMAT_UNDEFINED);
    m_extent = std::exchange(other.m_extent, VkExtent2D{0, 0});

    return *this;
  }

  bool init(VkPhysicalDevice physicalDevice, VkDevice device,
            VkExtent2D extent);
  void shutdown() noexcept;

  [[nodiscard]] VkImageView view() const noexcept { return m_view; }
  [[nodiscard]] VkFormat format() const noexcept { return m_format; }
  [[nodiscard]] bool valid() const noexcept { return m_view != VK_NULL_HANDLE; }

private:
  static bool hasStencil(VkFormat fmt) noexcept;
  static bool findSupportedDepthFormat(VkPhysicalDevice physicalDevice,
                                       VkFormat &out);

  VkDevice m_device = VK_NULL_HANDLE;    // non-owning
  VkImage m_image = VK_NULL_HANDLE;      // owning
  VkDeviceMemory m_mem = VK_NULL_HANDLE; // owning
  VkImageView m_view = VK_NULL_HANDLE;   // owning
  VkFormat m_format = VK_FORMAT_UNDEFINED;
  VkExtent2D m_extent{0, 0};
};
