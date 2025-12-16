#pragma once

#include <utility>
#include <vulkan/vulkan_core.h>

class VkRenderPassObj {
public:
  VkRenderPassObj() = default;
  ~VkRenderPassObj() noexcept { shutdown(); }

  VkRenderPassObj(const VkRenderPassObj &) = delete;
  VkRenderPassObj &operator=(const VkRenderPassObj &) = delete;

  VkRenderPassObj(VkRenderPassObj &&other) noexcept {
    *this = std::move(other);
  }
  VkRenderPassObj &operator=(VkRenderPassObj &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_renderPass = std::exchange(other.m_renderPass, VK_NULL_HANDLE);
    return *this;
  }

  bool init(VkDevice device, VkFormat swapchainColorFormat);
  void shutdown() noexcept;

  [[nodiscard]] VkRenderPass handle() const { return m_renderPass; }
  [[nodiscard]] bool valid() const { return m_renderPass != VK_NULL_HANDLE; }

private:
  VkDevice m_device = VK_NULL_HANDLE;         // non-owning
  VkRenderPass m_renderPass = VK_NULL_HANDLE; // owning
};
