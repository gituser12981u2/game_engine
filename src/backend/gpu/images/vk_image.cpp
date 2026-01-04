#include "backend/gpu/images/vk_image.hpp"

#include <cstdint>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkImageObj::init2D(VmaAllocator allocator, uint32_t width, uint32_t height,
                        VkFormat format, VkImageUsageFlags usage,
                        VkImageTiling tiling) {
  if (allocator == nullptr || width == 0 || height == 0 ||
      format == VK_FORMAT_UNDEFINED) {
    std::cerr << "[Image] init2D invalid args\n";
    return false;
  }

  shutdown();

  m_allocator = allocator;
  m_format = format;
  m_width = width;
  m_height = height;

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent = VkExtent3D{width, height, 1U};
  imageInfo.mipLevels = 1; // Todo: add mipmap
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // GPU only intent

  VkResult res = vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image,
                                &m_allocation, nullptr);
  if (res != VK_SUCCESS) {
    std::cerr << "[Image] vkCreateImage failed: " << res << "\n";
    shutdown();
    return false;
  }

  return true;
}

void VkImageObj::shutdown() noexcept {
  if (m_allocator != nullptr && m_image != VK_NULL_HANDLE &&
      m_allocation != nullptr) {
    vmaDestroyImage(m_allocator, m_image, m_allocation);
  }

  m_image = VK_NULL_HANDLE;
  m_allocation = nullptr;
  m_allocator = nullptr;
  m_format = VK_FORMAT_UNDEFINED;
  m_width = 0;
  m_height = 0;
}
