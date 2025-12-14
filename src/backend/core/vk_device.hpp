#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_core.h>

struct VkQueues {
  VkQueue graphics = VK_NULL_HANDLE;
  uint32_t graphicsFamily = UINT32_MAX;

  // TODO: add surface aware device selection
};

class VkDeviceCtx {
public:
  VkDeviceCtx() = default;
  ~VkDeviceCtx() { shutdown(); }

  VkDeviceCtx(const VkDeviceCtx &) = delete;
  VkDeviceCtx &operator=(const VkDeviceCtx &) = delete;

  bool init(VkInstance instance);
  void shutdown();

  VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
  VkDevice device() const { return m_device; }
  const VkQueues &queues() const { return m_queues; }

private:
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    bool isComplete() const { return graphicsFamily.has_value(); }
  };

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  bool pickPhysicalDevice(VkInstance instance);
  bool createLogicalDevice();

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueues m_queues{};
};
