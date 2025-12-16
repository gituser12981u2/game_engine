#pragma once

#include "vk_swapchain.hpp"

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

struct GLFWwindow;

// Owns VulkanSwapchain and VkSurfaceKHR
class VkPresenter {
public:
  VkPresenter() = default;
  ~VkPresenter() noexcept { shutdown(); }

  VkPresenter(const VkPresenter &) = delete;
  VkPresenter &operator=(const VkPresenter &) = delete;

  VkPresenter(VkPresenter &&other) noexcept { *this = std::move(other); }
  VkPresenter &operator=(VkPresenter &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_instance = std::exchange(other.m_instance, VK_NULL_HANDLE);
    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_surface = std::exchange(other.m_surface, VK_NULL_HANDLE);
    m_swapchain = std::move(other.m_swapchain);

    return *this;
  }

  bool init(VkInstance instance, VkPhysicalDevice physicalDevice,
            VkDevice device, GLFWwindow *window, uint32_t width,
            uint32_t height, uint32_t graphicsQueueFamilyIndex);
  void shutdown() noexcept;

  [[nodiscard]] VkFormat imageFormat() const {
    return m_swapchain.swapchainImageFormat();
  }
  [[nodiscard]] VkExtent2D extent() const {
    return m_swapchain.swapchainExtent();
  }
  [[nodiscard]] const std::vector<VkImageView> &imageViews() const {
    return m_swapchain.swapchainImageViews();
  }
  [[nodiscard]] VkSwapchainKHR swapchain() const {
    return m_swapchain.swapchain();
  }
  [[nodiscard]] VkSurfaceKHR surface() const { return m_surface; }

  [[nodiscard]] bool isInitialized() const {
    return m_surface != VK_NULL_HANDLE;
  }

private:
  VkInstance m_instance = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSwapchain m_swapchain;
};
