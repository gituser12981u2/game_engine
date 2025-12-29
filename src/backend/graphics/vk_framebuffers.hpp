#pragma once

#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkFramebuffers {
public:
  VkFramebuffers() = default;
  ~VkFramebuffers() noexcept { shutdown(); }

  VkFramebuffers(const VkFramebuffers &) = delete;
  VkFramebuffers &operator=(const VkFramebuffers &) = delete;

  VkFramebuffers(VkFramebuffers &&other) noexcept { *this = std::move(other); }
  VkFramebuffers &operator=(VkFramebuffers &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_swapchainFramebuffers = std::exchange(other.m_swapchainFramebuffers, {});
    return *this;
  }

  bool init(VkDevice device, VkRenderPass renderPass,
            std::span<const VkImageView> colorViews,
            std::span<const VkImageView> depthViews, VkExtent2D extent);
  void shutdown() noexcept;

  [[nodiscard]] const std::vector<VkFramebuffer> &handles() const noexcept {
    return m_swapchainFramebuffers;
  }
  [[nodiscard]] VkFramebuffer at(size_t i) const noexcept {
    return m_swapchainFramebuffers[i];
  }
  [[nodiscard]] size_t size() const noexcept {
    return m_swapchainFramebuffers.size();
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;                 // non-owning
  std::vector<VkFramebuffer> m_swapchainFramebuffers; // owning
};
