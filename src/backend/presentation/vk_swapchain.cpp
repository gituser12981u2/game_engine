#include "vk_swapchain.hpp"

#include "engine/logging/log.hpp"

#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <sys/stat.h>
#include <vector>
#include <vulkan/vulkan_core.h>

VkSwapchain::SwapchainSupportDetails
VkSwapchain::querySwapChainSupport(VkPhysicalDevice device,
                                   VkSurfaceKHR surface) {
  SwapchainSupportDetails details;

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

  LOGD("Swapchain Capabilites: minImageCount={}, maxImageCount={}, "
       "currentExtent=({}x{})",
       details.capabilities.minImageCount, details.capabilities.maxImageCount,
       details.capabilities.currentExtent.width,
       details.capabilities.currentExtent.height);

  LOGT("Available swapchain formats: {}", formatCount);
  for (const auto &f : details.formats) {
    LOGT("  format={} colorSpace={}", fmt::underlying(f.format),
         fmt::underlying(f.colorSpace));
  }

  LOGT("Available swapchain present modes {}", presentModeCount);
  for (const auto &pm : details.presentModes) {
    LOGT("presentMode={}", fmt::underlying(pm));
  }

  return details;
}

VkSurfaceFormatKHR VkSwapchain::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  if (availableFormats.size() == 1 &&
      availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    VkSurfaceFormatKHR format{};
    format.format = VK_FORMAT_B8G8R8A8_SRGB;
    format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    LOGI("Chose prefered format: {} / {}", fmt::underlying(format.format),
         fmt::underlying(format.colorSpace));
    return format;
  }

  // Prefer BGRA8 and sRGB_NONLINEAR
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
        availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
      LOGI("Chose BGRA8 + sRGB_NONLINEAR format: {} / {}",
           fmt::underlying(availableFormat.format),
           fmt::underlying(availableFormat.colorSpace));
      return availableFormat;
    }
  }

  // Then, prefer UNORM with SRGB_NONLINEAR
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
        availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
      LOGI("Chose UNORM + sRGB_NONLINEAR: {} / {}",
           fmt::underlying(availableFormat.format),
           fmt::underlying(availableFormat.colorSpace));
      return availableFormat;
    }
  }

  // Fallback to first available format
  LOGI("Using fallback format: {} / {}",
       fmt::underlying(availableFormats[0].format),
       fmt::underlying(availableFormats[0].colorSpace));
  return availableFormats[0];
}

VkPresentModeKHR VkSwapchain::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  // Prefer MAILBOX if present, also known as triple buffering
  // MoltenVK does not support MAILBOX. See Bill Holling's comment on the issue
  // (https://github.com/KhronosGroup/MoltenVK/issues/581#issuecomment-488903202)
  for (const auto &presentMode : availablePresentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      LOGI("Chose present mode MAILBOX");
      return presentMode;
    }
  }

  // Fallback to FIFO since it is guaranteed to be supported
  for (const auto &presentMode : availablePresentModes) {
    if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
      LOGI("Chose present mode FIFO");
      return presentMode;
    }
  }

  // As a last resort, pick the first
  VkPresentModeKHR fallback = availablePresentModes.empty()
                                  ? VK_PRESENT_MODE_FIFO_KHR
                                  : availablePresentModes[0];
  LOGI("Using fallback present mode {}", fmt::underlying(fallback));
  return fallback;
}

VkExtent2D
VkSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              uint32_t width, uint32_t height) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    // The surface size is dictated by the window system (common on macOS)
    return capabilities.currentExtent;
  }

  VkExtent2D actualExtent = {width, height};

  actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                 capabilities.maxImageExtent.width);
  actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height);

  LOGI("Using clamped extent: ({}x{})", actualExtent.width,
       actualExtent.height);
  return actualExtent;
}

bool VkSwapchain::init(VkBackendCtx &ctx, VkSurfaceKHR surface, uint32_t width,
                       uint32_t height) {
  if (surface == VK_NULL_HANDLE) {
    LOGE("Surface is null");
    return false;
  }

  VkDevice device = ctx.device();
  VkPhysicalDevice physicalDevice = ctx.physicalDevice();

  VkSwapchainKHR old = m_swapchain;

  LOGD("Destroying swapchainimage views");
  destroySwapchainImageViews(device);

  m_surface = surface;

  SwapchainSupportDetails support =
      querySwapChainSupport(physicalDevice, m_surface);

  if (support.formats.empty() || support.presentModes.empty()) {
    LOGE("Swapchain support incomplete");
    return false;
  }

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
  VkExtent2D extent = chooseSwapExtent(support.capabilities, width, height);

  uint32_t imageCount = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 &&
      imageCount > support.capabilities.maxImageCount) {
    LOGI("Clamping imageCount from {} to maxImageCount={}", imageCount,
         support.capabilities.maxImageCount);
    imageCount = support.capabilities.maxImageCount;
  }

  LOGI("Selected swapchain parameters: imageCount = {} colorSpace = {} extent "
       "= ({}x{})",
       imageCount, fmt::underlying(surfaceFormat.colorSpace), extent.width,
       extent.height);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = m_surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.presentMode = presentMode;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices = nullptr;
  // Can be set to IDENTITY for non desktop apps
  createInfo.preTransform = support.capabilities.currentTransform;
  // Usually opaque
  // TODO: add flag to check that alpha_opaque is available
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // Clipped pixels are rendered out
  createInfo.clipped = VK_TRUE;

  LOGI("Swapchain preTransform = {}, compositeAlpha = {}, clipped = {}",
       fmt::underlying(support.capabilities.currentTransform),
       fmt::underlying(createInfo.compositeAlpha), createInfo.clipped);

  createInfo.oldSwapchain = old;

  VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
  VkResult res =
      vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapchain);
  if (res != VK_SUCCESS) {
    LOGE("vkCreateSwapchainKHR failed: {}", fmt::underlying(res));
    return false;
  }

  // Destroy old swapchain
  if (old != VK_NULL_HANDLE) {
    LOGD("Destroying swapchain");
    vkDestroySwapchainKHR(device, old, nullptr);
  }

  m_swapchain = newSwapchain;

  vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, nullptr);
  m_swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount,
                          m_swapchainImages.data());

  m_swapchainImageFormat = surfaceFormat.format;
  m_swapchainExtent = extent;

  return true;
}

void VkSwapchain::shutdown(VkDevice device) noexcept {
  if (device != VK_NULL_HANDLE) {
    LOGD("Destroying swapchain image views");
    destroySwapchainImageViews(device);

    if (m_swapchain != VK_NULL_HANDLE) {
      LOGD("Destroying swapchain");
      vkDestroySwapchainKHR(device, m_swapchain, nullptr);
    }
  } else {
    m_swapchainImageViews.clear();
  }

  m_swapchain = VK_NULL_HANDLE;
  m_swapchainImages.clear();

  m_surface = VK_NULL_HANDLE;
  m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
  m_swapchainExtent = {};
}

bool VkSwapchain::createSwapchainImageViews(VkDevice device) {
  destroySwapchainImageViews(device);

  if (device == VK_NULL_HANDLE) {
    LOGE("Device is null");
    return false;
  }

  const auto &images = swapchainImages();
  if (images.empty()) {
    LOGE("Swapchain images are empty");
    return false;
  }

  VkFormat format = swapchainImageFormat();
  if (format == VK_FORMAT_UNDEFINED) {
    LOGE("Swapchain format undefined");
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
      LOGE("vkCreateImageView() failed at index {} error=", i,
           fmt::underlying(res));
      destroySwapchainImageViews(device);
      return false;
    }
  }

  return true;
}

void VkSwapchain::destroySwapchainImageViews(VkDevice device) noexcept {
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
