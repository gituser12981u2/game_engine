#include "vk_framebuffers.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

bool VkFramebuffers::init(VkDevice device, VkRenderPass renderPass,
                          std::span<const VkImageView> colorViews,
                          std::span<const VkImageView> depthViews,
                          VkExtent2D extent) {
  if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE) {
    std::cerr << "[Framebuffers] Device or render pass not ready\n";
    return false;
  }

  if (colorViews.empty()) {
    std::cerr << "[Framebuffers] No color image views.\n";
    return false;
  }

  if (depthViews.empty()) {
    std::cerr << "[Framebuffers] No depth image views.\n";
    return false;
  }

  if (colorViews.size() != depthViews.size()) {
    std::cerr << "[Framebuffers] Color/depth view count mismatch: color="
              << colorViews.size() << " depth=" << depthViews.size() << "\n";
    return false;
  }

  if (extent.width == 0 || extent.height == 0) {
    std::cerr << "[Framebuffers] Invalid extent\n";
    return false;
  }

  shutdown();
  m_device = device;

  m_swapchainFramebuffers.resize(colorViews.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < colorViews.size(); ++i) {
    std::array<VkImageView, 2> attachments = {colorViews[i], depthViews[i]};

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = extent.width;
    fbInfo.height = extent.height;
    fbInfo.layers = 1;

    VkResult res = vkCreateFramebuffer(m_device, &fbInfo, nullptr,
                                       &m_swapchainFramebuffers[i]);
    if (res != VK_SUCCESS) {
      std::cerr << "[Framebuffers] vkCreateFramebuffer() failed at index " << i
                << " error=" << res << "\n";
      shutdown();
      return false;
    }
  }

  std::cout << "[Framebuffers] Created " << m_swapchainFramebuffers.size()
            << " framebuffers\n";
  return true;
}

void VkFramebuffers::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    for (VkFramebuffer fb : m_swapchainFramebuffers) {
      if (fb != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device, fb, nullptr);
      }
    }
  }

  m_swapchainFramebuffers.clear();
  m_device = VK_NULL_HANDLE;
}
