#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
class VkCommands {
public:
  VkCommands() = default;
  ~VkCommands() { shutdown(); }

  VkCommands(const VkCommands &) = delete;
  VkCommands &operator=(const VkCommands &) = delete;

  bool init(VkDevice device, uint32_t queueFamilyIndex,
            VkCommandPoolCreateFlags flags =
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  void shutdown();

  bool allocate(uint32_t count,
                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  void free();

  VkCommandPool pool() const { return m_pool; }
  const std::vector<VkCommandBuffer> &buffers() const { return m_buffers; }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkCommandPool m_pool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_buffers;
};
