#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VkSwapchain {
public:
  VkSwapchain() = default;
  ~VkSwapchain() = default;

  VkSwapchain(const VkSwapchain &) = delete;
  VkSwapchain &operator=(const VkSwapchain &) = delete;

  VkSwapchain(VkSwapchain &&other) noexcept { *this = std::move(other); }
  VkSwapchain &operator=(VkSwapchain &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    m_surface = std::exchange(other.m_surface, VK_NULL_HANDLE);
    m_swapChain = std::exchange(other.m_swapChain, VK_NULL_HANDLE);
    m_swapChainImages = std::exchange(other.m_swapChainImages, {});
    m_swapChainImageViews = std::exchange(other.m_swapChainImageViews, {});
    m_swapChainImageFormat =
        std::exchange(other.m_swapChainImageFormat, VK_FORMAT_UNDEFINED);
    m_swapChainExtent = std::exchange(other.m_swapChainExtent, VkExtent2D{});
    return *this;
  }

  bool init(VkPhysicalDevice physicalDevice, VkDevice device,
            VkSurfaceKHR surface, uint32_t width, uint32_t height,
            uint32_t graphicsQueueFamilyIndex);
  void shutdown(VkDevice device) noexcept;

  [[nodiscard]] VkSwapchainKHR swapchain() const noexcept {
    return m_swapChain;
  }
  [[nodiscard]] VkFormat swapchainImageFormat() const noexcept {
    return m_swapChainImageFormat;
  }
  [[nodiscard]] VkExtent2D swapchainExtent() const noexcept {
    return m_swapChainExtent;
  }

  bool createSwapchainImageViews(VkDevice device);
  void destroySwapchainImageViews(VkDevice device) noexcept;

  [[nodiscard]] const std::vector<VkImage> &swapchainImages() const noexcept {
    return m_swapChainImages;
  }

  [[nodiscard]] const std::vector<VkImageView> &
  swapchainImageViews() const noexcept {
    return m_swapChainImageViews;
  }

private:
  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                       VkSurfaceKHR surface);
  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  static VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  static VkExtent2D
  chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width,
                   uint32_t height);

  VkSurfaceKHR m_surface = VK_NULL_HANDLE; // non-owning
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  std::vector<VkImage> m_swapChainImages;
  std::vector<VkImageView> m_swapChainImageViews;
  VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D m_swapChainExtent{};
};
