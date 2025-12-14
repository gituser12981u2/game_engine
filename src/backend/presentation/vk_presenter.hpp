#pragma once

#include "vk_swapchain.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

struct GLFWwindow;

// Owns VulkanSwapchain and VkSurfaceKHR
class VkPresenter {
public:
  VkPresenter() = default;
  ~VkPresenter() { shutdown(); }

  VkPresenter(const VkPresenter &) = delete;
  VkPresenter &operator=(const VkPresenter &) = delete;

  bool init(VkInstance instance, VkPhysicalDevice physicalDevice,
            VkDevice device, GLFWwindow *window, uint32_t width,
            uint32_t height, uint32_t graphicsQueueFamilyIndex);
  void shutdown();

  VkFormat imageFormat() const { return m_swapchain.swapchainImageFormat(); }
  VkExtent2D extent() const { return m_swapchain.swapchainExtent(); }
  const std::vector<VkImageView> &imageViews() const {
    return m_swapchain.swapchainImageViews();
  }
  VkSwapchainKHR swapchain() const { return m_swapchain.swapchain(); }
  VkSurfaceKHR surface() const { return m_surface; }

  bool isInitialized() const { return m_surface != VK_NULL_HANDLE; }

private:
  VkInstance m_instance = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSwapchain m_swapchain;
};
