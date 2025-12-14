#include "vk_swapchain.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

VkSwapchain::SwapChainSupportDetails
VkSwapchain::querySwapChainSupport(VkPhysicalDevice device,
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

VkSurfaceFormatKHR VkSwapchain::chooseSwapSurfaceFormat(
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

VkPresentModeKHR VkSwapchain::chooseSwapPresentMode(
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
VkSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
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

bool VkSwapchain::init(VkPhysicalDevice physicalDevice, VkDevice device,
                       VkSurfaceKHR surface, uint32_t width, uint32_t height,
                       uint32_t graphicsQueueFamilyIndex) {
  // Re-init
  shutdown(device);

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
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices = nullptr;

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

void VkSwapchain::shutdown(VkDevice device) {
  std::cout << "[Swapchain] shutdown has begun\n";

  destroySwapchainImageViews(device);

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

bool VkSwapchain::createSwapchainImageViews(VkDevice device) {
  // If called twice, clean up first
  destroySwapchainImageViews(device);

  if (device == VK_NULL_HANDLE) {
    std::cerr << "[ImageViews] Device is null\n";
    return false;
  }

  const auto &images = swapchainImages();
  if (images.empty()) {
    std::cerr << "[ImageViews] Swapchain images are empty\n";
    return false;
  }

  VkFormat format = swapchainImageFormat();
  if (format == VK_FORMAT_UNDEFINED) {
    std::cerr << "[ImageViews] Swapchain format undefined\n";
    return false;
  }

  m_swapchainImageViews.resize(images.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < images.size(); ++i) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult res = vkCreateImageView(device, &viewInfo, nullptr,
                                     &m_swapchainImageViews[i]);
    if (res != VK_SUCCESS) {
      std::cerr << "[Swapchain] vkCreateImageView() failed at index " << i
                << " error=" << res << "\n";
      // destroy any created so far
      destroySwapchainImageViews(device);
      return false;
    }
  }

  std::cout << "[Swapchain] Created " << m_swapchainImageViews.size()
            << " swapchain image views\n";
  return true;
}

void VkSwapchain::destroySwapchainImageViews(VkDevice device) {
  if (device == VK_NULL_HANDLE) {
    m_swapchainImageViews.clear();
    return;
  }

  for (VkImageView v : m_swapchainImageViews) {
    if (v != VK_NULL_HANDLE) {
      vkDestroyImageView(device, v, nullptr);
    }
  }
  m_swapchainImageViews.clear();
}
