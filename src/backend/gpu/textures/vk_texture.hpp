#pragma once

#include "backend/gpu/images/vk_image.hpp"

#include <vulkan/vulkan_core.h>

struct VkTexture2D {
  VkDevice device = VK_NULL_HANDLE;
  VkImageObj image;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  void shutdown() noexcept {
    if (device != VK_NULL_HANDLE) {
      if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
      }

      if (view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, view, nullptr);
      }
    }

    sampler = VK_NULL_HANDLE;
    view = VK_NULL_HANDLE;
    image.shutdown();
    device = VK_NULL_HANDLE;
  }

  [[nodiscard]] bool valid() const noexcept {
    return image.valid() && view != VK_NULL_HANDLE && sampler != VK_NULL_HANDLE;
  }
};
