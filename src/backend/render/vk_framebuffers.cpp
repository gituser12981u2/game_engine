#include "vk_framebuffers.hpp"

#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

bool VkFramebuffers::init(VkDevice device, VkRenderPass renderPass,

                          const std::vector<VkImageView> &swapchainImageViews,
                          VkExtent2D extent) {
  if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE) {
    std::cerr << "[Framebuffers] Device or render pass not ready\n";
    return false;
  }

  if (swapchainImageViews.empty()) {
    std::cerr << "[Framebuffers] No swapchain image views.\n";
    return false;
  }

  if (extent.width == 0 || extent.height == 0) {
    std::cerr << "[Framebuffers] Invalid extent\n";
    return false;
  }

  // Re-init
  shutdown();
  m_device = device;

  m_swapchainFramebuffers.resize(swapchainImageViews.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
    VkImageView attachments[] = {swapchainImageViews[i]};

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = attachments;
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

void VkFramebuffers::shutdown() {
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
