#include "backend/gpu/images/vk_depth_image.hpp"

#include "backend/gpu/images/vk_image.hpp"
#include "engine/logging/log.hpp"

#include <array>
#include <fmt/format.h>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkDepthImage::hasStencil(VkFormat fmt) noexcept {
  return fmt == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         fmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool VkDepthImage::findSupportedDepthFormat(VkPhysicalDevice physicalDevice,
                                            VkFormat &out) {
  const std::array<VkFormat, 3> candidates = {VK_FORMAT_D32_SFLOAT,
                                              VK_FORMAT_D32_SFLOAT_S8_UINT,
                                              VK_FORMAT_D24_UNORM_S8_UINT};

  // TODO: pick preferred format instead first one
  for (VkFormat fmt : candidates) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &props);

    const auto features = props.optimalTilingFeatures;
    if ((features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
      out = fmt;
      LOGD("Chosen depth format: {}", static_cast<int>(out));
      return true;
    }
  }

  return false;
}

bool VkDepthImage::init(VkBackendCtx &ctx, VkExtent2D extent) {
  if (extent.width == 0 || extent.height == 0) {
    LOGE("Invalid init args");
    std::cerr << "[Depth] Invalid init args\n";
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_extent = extent;

  if (!findSupportedDepthFormat(m_ctx->physicalDevice(), m_format)) {
    std::cerr << "[Depth] No supported depth format found\n";
    shutdown();
    return false;
  }

  if (!m_image.init2D(m_ctx->allocator(), extent.width, extent.height, m_format,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_IMAGE_TILING_OPTIMAL)) {
    std::cerr << "[Depth] Failed to create depth image\n";
    shutdown();
    return false;
  }

  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (hasStencil(m_format)) {
    aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = m_image.handle();
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = m_format;
  viewInfo.subresourceRange.aspectMask = aspect;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  const VkResult res =
      vkCreateImageView(m_ctx->device(), &viewInfo, nullptr, &m_view);
  if (res != VK_SUCCESS) {
    LOGE("vkCreateImageView failed: {}", fmt::underlying(res));
    shutdown();
    return false;
  }

  return true;
}

void VkDepthImage::shutdown() noexcept {
  VkDevice device = VK_NULL_HANDLE;
  if (m_ctx != nullptr) {
    device = m_ctx->device();
  }

  if (device != VK_NULL_HANDLE && m_view != VK_NULL_HANDLE) {
    LOGD("Destroying image view");
    vkDestroyImageView(m_ctx->device(), m_view, nullptr);
  }

  m_view = VK_NULL_HANDLE;
  m_image.shutdown();
  m_format = VK_FORMAT_UNDEFINED;
  m_extent = VkExtent2D{.width = 0, .height = 0};
  m_ctx = nullptr;
}
