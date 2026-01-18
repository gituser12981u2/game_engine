#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/images/vk_image.hpp"

#include <utility>
#include <vk_mem_alloc.h>
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

    m_ctx = std::exchange(other.m_ctx, nullptr);
    m_image = std::move(other.m_image);
    m_view = std::exchange(other.m_view, VK_NULL_HANDLE);
    m_format = std::exchange(other.m_format, VK_FORMAT_UNDEFINED);
    m_extent = std::exchange(other.m_extent, VkExtent2D{0, 0});

    return *this;
  }

  bool init(VkBackendCtx &ctx, VkExtent2D extent);
  void shutdown() noexcept;

  [[nodiscard]] VkImage image() const noexcept { return m_image.handle(); }
  [[nodiscard]] VkImageView view() const noexcept { return m_view; }
  [[nodiscard]] VkFormat format() const noexcept { return m_format; }
  [[nodiscard]] bool valid() const noexcept { return m_view != VK_NULL_HANDLE; }

private:
  static bool hasStencil(VkFormat fmt) noexcept;
  static bool findSupportedDepthFormat(VkPhysicalDevice physicalDevice,
                                       VkFormat &out);

  VkBackendCtx *m_ctx = nullptr;       // non-owning
  VkImageObj m_image;                  // owning
  VkImageView m_view = VK_NULL_HANDLE; // owning
  VkFormat m_format = VK_FORMAT_UNDEFINED;
  VkExtent2D m_extent{0, 0};
};
