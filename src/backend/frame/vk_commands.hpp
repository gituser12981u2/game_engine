#pragma once

#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkCommands {
public:
  VkCommands() = default;
  ~VkCommands() noexcept { shutdown(); }

  VkCommands(const VkCommands &) = delete;
  VkCommands &operator=(const VkCommands &) = delete;

  VkCommands(VkCommands &&other) noexcept { *this = std::move(other); }
  VkCommands &operator=(VkCommands &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
    m_buffers = std::exchange(other.m_buffers, std::vector<VkCommandBuffer>{});
    return *this;
  }

  bool init(VkDevice device, uint32_t queueFamilyIndex,
            VkCommandPoolCreateFlags flags =
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  void shutdown() noexcept;

  bool allocate(uint32_t count,
                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  void free() noexcept;

  [[nodiscard]] VkCommandPool pool() const noexcept { return m_pool; }
  [[nodiscard]] const std::vector<VkCommandBuffer> &buffers() const noexcept {
    return m_buffers;
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;     // no-owning
  VkCommandPool m_pool = VK_NULL_HANDLE;  // owning
  std::vector<VkCommandBuffer> m_buffers; // allocated
};
