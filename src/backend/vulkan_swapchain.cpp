#include "vulkan_swapchain.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

VulkanSwapchain::SwapChainSupportDetails
VulkanSwapchain::querySwapChainSupport(VkPhysicalDevice device,
                                       VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }

  std::cout << "[Swapchain] Capabilites:"
            << " minImageCount=" << details.capabilities.minImageCount
            << " maxImageCount=" << details.capabilities.maxImageCount
            << " currentExtent=(" << details.capabilities.currentExtent.width
            << "x" << details.capabilities.currentExtent.height << ")\n";

  std::cout << "[Swapchain] Available formats: " << formatCount << "\n";
  for (const auto &f : details.formats) {
    std::cout << " format=" << f.format << " colorSpace=" << f.colorSpace
              << "\n";
  }

  std::cout << "[Swapchain] Available present modes: " << presentModeCount
            << "\n";
  for (const auto &pm : details.presentModes) {
    std::cout << " presentMode=" << pm << "\n";
  }

  return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  // If architecture allows choosing of any format
  if (availableFormats.size() == 1 &&
      availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    VkSurfaceFormatKHR format{};
    format.format = VK_FORMAT_B8G8R8A8_SRGB;
    format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    std::cout << "[Swapchain] Chose perferred format (UNDEFINED -> default): "
              << format.format << " / " << format.colorSpace << "\n";
    return format;
  }

  // Prefer BGRA8 and sRGB_NONLINEAR
  // TODO: Make this user defined through a public API
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
        availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
      std::cout << "[Swapchain] Chose perferred sRGB format: "
                << availableFormat.format << " / " << availableFormat.colorSpace
                << "\n";
      return availableFormat;
    }
  }

  // Then, prefer UNORM with SRGB_NONLINEAR
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
        availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
      std::cout << "[Swapchain] Chose UNORM + sRGB_NONLINEAR: "
                << availableFormat.format << " / " << availableFormat.colorSpace
                << "\n";
      return availableFormat;
    }
  }

  // Fallback to first available format
  std::cout << "[Swapchain] Using fallback format: "
            << availableFormats[0].format << " / "
            << availableFormats[0].colorSpace << "\n";
  return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  // Prefer MAILBOX if present, also known as triple buffering
  // MoltenVK does not support MAILBOX. See Bill Holling's comment on the issue
  // (https://github.com/KhronosGroup/MoltenVK/issues/581#issuecomment-488903202)
  for (const auto &presentMode : availablePresentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      std::cout << "[Swapchain] Chose present mode: MAILBOX\n";
      return presentMode;
    }
  }

  // Fallback: FIFO since it is guaranteed to be supported
  for (const auto &presentMode : availablePresentModes) {
    if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
      std::cout << "[Swapchain] Chose present mode: FIFO\n";
      return presentMode;
    }
  }

  // As a last resort, pick the first
  VkPresentModeKHR fallback = availablePresentModes.empty()
                                  ? VK_PRESENT_MODE_FIFO_KHR
                                  : availablePresentModes[0];
  std::cout << "[Swapchain] Using fallback present mode: " << fallback << "\n";
  return fallback;
}

VkExtent2D
VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                                  uint32_t width, uint32_t height) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    std::cout << "[Swapchain] Using currentExtent from capabilities: ("
              << capabilities.currentExtent.width << "x"
              << capabilities.currentExtent.height << ")\n";
    // The surface size is dictated by the window system (common on macOS)
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {width, height};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    std::cout << "[Swapchain] Using clamped extent: (" << actualExtent.width
              << "x" << actualExtent.height << ")\n";
    return actualExtent;
  }
}

bool VulkanSwapchain::init(VkPhysicalDevice physicalDevice, VkDevice device,
                           VkSurfaceKHR surface, uint32_t width,
                           uint32_t height, uint32_t graphicsQueueFamilyIndex) {
  std::cout << "[Swapchain] initialization has begun\n";

  m_surface = surface;

  SwapChainSupportDetails support =
      querySwapChainSupport(physicalDevice, m_surface);

  if (support.formats.empty() || support.presentModes.empty()) {
    std::cerr << "Swapchain support incomplete\n";
    return false;
  }

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
  VkExtent2D extent = chooseSwapExtent(support.capabilities, width, height);

  uint32_t imageCount = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 &&
      imageCount > support.capabilities.maxImageCount) {
    std::cout << "[Swapchain] Clamping imageCount from" << imageCount
              << " to maxImageCount=" << support.capabilities.maxImageCount
              << "\n";
    imageCount = support.capabilities.maxImageCount;
  }

  std::cout << "[Swapchain] Selected parameters:\n";
  std::cout << "  imageCount = " << imageCount << "\n";
  std::cout << "  colorSpace = " << surfaceFormat.colorSpace << "\n";
  std::cout << "  extent     = (" << extent.width << "x" << extent.height
            << ")\n";

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = m_surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {graphicsQueueFamilyIndex};
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 1;
  createInfo.pQueueFamilyIndices = queueFamilyIndices;

  // Can be set to IDENTITY for non desktop apps
  createInfo.preTransform = support.capabilities.currentTransform;
  // Usually opaque
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // Clipped pixels are rendered out
  createInfo.clipped = VK_TRUE;
  // TODO: Handle reusing swapchain for resizing of surface.
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  std::cout << "[Swapchain] preTransform = "
            << support.capabilities.currentTransform << "\n";
  std::cout
      << "[Swapchain] compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR\n";
  std::cout << "[Swapchain] clipped = VK_TRUE\n";

  VkResult result =
      vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapChain);
  if (result != VK_SUCCESS) {
    std::cerr << "[Swapchain] ERROR: vkCreateSwapchainKHR failed: " << result
              << "\n";
    return false;
  }

  vkGetSwapchainImagesKHR(device, m_swapChain, &imageCount, nullptr);
  m_swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, m_swapChain, &imageCount,
                          m_swapChainImages.data());

  m_swapChainImageFormat = surfaceFormat.format;
  m_swapChainExtent = extent;

  std::cout << "[Swapchain] initialized success," << m_swapChainImages.size()
            << " images acquired\n";
  return true;
}

void VulkanSwapchain::shutdown(VkDevice device) {
  std::cout << "[Swapchain] shutdown has begun\n";

  if (m_swapChain != VK_NULL_HANDLE) {
    std::cout << "[Swapchain] Destroying swapchain\n";
    vkDestroySwapchainKHR(device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
    m_swapChainImages.clear();
  } else {
    std::cout << "[Swapchain] No swapchain to destory\n";
  }

  // Surface is owned by whomever created it
  m_surface = VK_NULL_HANDLE;

  std::cout << "[Swapchain] shutdown completed\n";
}
