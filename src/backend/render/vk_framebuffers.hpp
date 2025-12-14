#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class VkFramebuffers {
public:
  VkFramebuffers() = default;
  ~VkFramebuffers() { shutdown(); }

  VkFramebuffers(const VkFramebuffers &) = delete;
  VkFramebuffers &operator=(const VkFramebuffers &) = delete;

  bool init(VkDevice device, VkRenderPass renderPass,
            const std::vector<VkImageView> &swapchainImageViews,
            VkExtent2D extent);
  void shutdown();

  const std::vector<VkFramebuffer> &handles() const {
    return m_swapchainFramebuffers;
  }
  VkFramebuffer at(size_t i) const { return m_swapchainFramebuffers[i]; }
  size_t size() const { return m_swapchainFramebuffers.size(); }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> m_swapchainFramebuffers;
};
