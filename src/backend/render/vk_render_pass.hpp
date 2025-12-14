#pragma once

#include <vulkan/vulkan_core.h>
class VkRenderPassObj {
public:
  VkRenderPassObj() = default;
  ~VkRenderPassObj() { shutdown(); }

  VkRenderPassObj(const VkRenderPassObj &) = delete;
  VkRenderPassObj &operator=(const VkRenderPassObj &) = delete;

  bool init(VkDevice device, VkFormat swapchainColorFormat);
  void shutdown();

  VkRenderPass handle() const { return m_renderPass; }
  bool valid() const { return m_renderPass != VK_NULL_HANDLE; }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
};
