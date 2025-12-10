#pragma once

#include "vulkan_swapchain.hpp"
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VulkanContext {
public:
  VulkanContext();
  ~VulkanContext();

  bool
  init(const std::vector<const char *>
           &platformExtensions); // Create instance, debug messenger, and device
  void shutdown();               // Destroy in reverse order

  VkInstance instance() const { return m_instance; }
  VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
  VkDevice device() const { return m_device; }
  VkQueue graphicsQueue() const { return m_graphicsQueue; }
  uint32_t graphicsQueueFamilyIndex() const {
    return m_graphicsQueueFamilyIndex;
  }

  bool createSwapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    return m_swapchain.init(m_physicalDevice, m_device, surface, width, height,
                            m_graphicsQueueFamilyIndex);
  }

  void destroySwapchain() { m_swapchain.shutdown(m_device); }

  const VulkanSwapchain &swapchain() const { return m_swapchain; }

private:
  bool checkValidationLayerSupport();
  bool createInstance(const std::vector<const char *> &platformExtensions);
  bool setupDebugMessenger();

  bool pickPhysicalDevice();
  bool createLogicalDevice();

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() const { return graphicsFamily.has_value(); }
  };

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;

  VulkanSwapchain m_swapchain;

  bool m_enableValidationLayers = true; // Gated by NDEBUG in cpp
};
