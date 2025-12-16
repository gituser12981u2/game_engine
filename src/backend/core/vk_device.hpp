#pragma once

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

struct VkQueues {
  VkQueue graphics = VK_NULL_HANDLE;
  uint32_t graphicsFamily = UINT32_MAX;

  // TODO: add surface aware device selection
};

class VkDeviceCtx {
public:
  VkDeviceCtx() = default;
  ~VkDeviceCtx() noexcept { shutdown(); }

  VkDeviceCtx(const VkDeviceCtx &) = delete;
  VkDeviceCtx &operator=(const VkDeviceCtx &) = delete;

  VkDeviceCtx(VkDeviceCtx &&other) noexcept { *this = std::move(other); }
  VkDeviceCtx &operator=(VkDeviceCtx &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_physicalDevice = std::exchange(other.m_physicalDevice, VK_NULL_HANDLE);
    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_queues = std::exchange(other.m_queues, VkQueues{});
    return *this;
  }

  bool init(VkInstance instance);
  void shutdown() noexcept;

  [[nodiscard]] VkPhysicalDevice physicalDevice() const {
    return m_physicalDevice;
  }
  [[nodiscard]] VkDevice device() const { return m_device; }
  [[nodiscard]] const VkQueues &queues() const { return m_queues; }

private:
  [[nodiscard]] bool pickPhysicalDevice(VkInstance instance);
  [[nodiscard]] bool createLogicalDevice();

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueues m_queues{};
};
