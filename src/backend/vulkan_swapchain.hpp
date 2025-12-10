#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VulkanSwapchain {
public:
  VulkanSwapchain() = default;
  ~VulkanSwapchain() = default;

  bool init(VkPhysicalDevice physicalDevice, VkDevice device,
            VkSurfaceKHR surface, uint32_t width, uint32_t height,
            uint32_t graphicsQueueFamilyIndex);
  void shutdown(VkDevice device);

  VkSwapchainKHR swapchain() const { return m_swapChain; }
  VkFormat swapchainImageFormat() const { return m_swapChainImageFormat; }
  VkExtent2D swapchainExtent() const { return m_swapChainExtent; }
  const std::vector<VkImage> &swapchainImages() const {
    return m_swapChainImages;
  }

private:
  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                VkSurfaceKHR surface);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              uint32_t width, uint32_t height);

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  std::vector<VkImage> m_swapChainImages;
  VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D m_swapChainExtent{};
};
