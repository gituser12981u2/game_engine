#include "vk_depth_image.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

static bool findMemoryTypeIndex(VkPhysicalDevice physicalDevice,
                                uint32_t typeBits, VkMemoryPropertyFlags props,
                                uint32_t &outIndex) {
  VkPhysicalDeviceMemoryProperties memProps{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    const uint32_t bit = (1U << i);
    const bool typeOk = (typeBits & bit) != 0;
    const bool propsOk =
        (memProps.memoryTypes[i].propertyFlags & props) == props;
    if (typeOk && propsOk) {
      outIndex = i;
      return true;
    }
  }

  return false;
}

bool VkDepthImage::hasStencil(VkFormat fmt) noexcept {
  return fmt == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         fmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool VkDepthImage::findSupportedDepthFormat(VkPhysicalDevice physicalDevice,
                                            VkFormat &out) {
  const std::array<VkFormat, 3> candidates = {VK_FORMAT_D32_SFLOAT,
                                              VK_FORMAT_D32_SFLOAT_S8_UINT,
                                              VK_FORMAT_D24_UNORM_S8_UINT};

  // TODO: pick preferred format instead first one
  for (VkFormat fmt : candidates) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &props);

    const auto features = props.optimalTilingFeatures;
    if ((features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
      out = fmt;
      std::cout << "[Depth] Chosen depth format: " << static_cast<int>(out)
                << "\n";
      return true;
    }
  }

  return false;
}

bool VkDepthImage::init(VkPhysicalDevice physicalDevice, VkDevice device,
                        VkExtent2D extent) {
  if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE ||
      extent.width == 0 || extent.height == 0) {
    std::cerr << "[Depth] Invalid init args\n";
    return false;
  }

  // Re-init
  shutdown();

  m_device = device;
  m_extent = extent;

  if (!findSupportedDepthFormat(physicalDevice, m_format)) {
    std::cerr << "[Depth] No supported depth format found\n";
    shutdown();
    return false;
  }

  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent = VkExtent3D{extent.width, extent.height, .depth = 1};
  imageInfo.mipLevels = 1; // Todo: add mipmap
  imageInfo.arrayLayers = 1;
  imageInfo.format = m_format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult res = vkCreateImage(m_device, &imageInfo, nullptr, &m_image);
  if (res != VK_SUCCESS) {
    std::cerr << "[Depth] vkCreateImage failed: " << res << "\n";
    shutdown();
    return false;
  }

  VkMemoryRequirements memReq{};
  vkGetImageMemoryRequirements(m_device, m_image, &memReq);

  uint32_t memIndex = 0;
  if (!findMemoryTypeIndex(physicalDevice, memReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memIndex)) {
    std::cerr << "[Depth] No suitable memory type\n";
    shutdown();
    return false;
  }

  VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memReq.size;
  allocInfo.memoryTypeIndex = memIndex;

  res = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_mem);
  if (res != VK_SUCCESS) {
    std::cerr << "[Depth] vkBindImageMemory failed: " << res << "\n";
    shutdown();
    return false;
  }

  res = vkBindImageMemory(m_device, m_image, m_mem, 0);
  if (res != VK_SUCCESS) {
    std::cerr << "[Depth] vkBindImageMemory failed: " << res << "\n";
    shutdown();
    return false;
  }

  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (hasStencil(m_format)) {
    aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = m_image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = m_format;
  viewInfo.subresourceRange.aspectMask = aspect;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  res = vkCreateImageView(m_device, &viewInfo, nullptr, &m_view);
  if (res != VK_SUCCESS) {
    std::cerr << "[Depth] vkCreateImageView failed: " << res << "\n";
    shutdown();
    return false;
  }

  return true;
}

void VkDepthImage::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    if (m_view != VK_NULL_HANDLE) {
      vkDestroyImageView(m_device, m_view, nullptr);
    }

    if (m_image != VK_NULL_HANDLE) {
      vkDestroyImage(m_device, m_image, nullptr);
    }

    if (m_mem != VK_NULL_HANDLE) {
      vkFreeMemory(m_device, m_mem, nullptr);
    }
  }

  m_view = VK_NULL_HANDLE;
  m_image = VK_NULL_HANDLE;
  m_mem = VK_NULL_HANDLE;
  m_format = VK_FORMAT_UNDEFINED;
  m_extent = VkExtent2D{.width = 0, .height = 0};
  m_device = VK_NULL_HANDLE;
}
