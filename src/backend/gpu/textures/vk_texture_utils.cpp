#include "backend/gpu/textures/vk_texture_utils.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>

bool vkCreateTextureView(VkDevice device, VkImage image, VkFormat format,
                         VkImageView &outView) {
  outView = VK_NULL_HANDLE;

  VkImageViewCreateInfo imageViewInfo{};
  imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewInfo.image = image;
  imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewInfo.format = format;
  imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewInfo.subresourceRange.baseMipLevel = 0;
  imageViewInfo.subresourceRange.levelCount = 1;
  imageViewInfo.subresourceRange.baseArrayLayer = 0;
  imageViewInfo.subresourceRange.layerCount = 1;

  const VkResult res =
      vkCreateImageView(device, &imageViewInfo, nullptr, &outView);
  if (res != VK_SUCCESS) {
    std::cerr << "[Texture] vkCreateImageView failed: " << res << "\n";
    outView = VK_NULL_HANDLE;
    return false;
  }

  return true;
}

bool vkCreateTextureSampler(VkDevice device, VkSampler &outSampler) {
  outSampler = VK_NULL_HANDLE;

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0F;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.minLod = 0.0F;
  samplerInfo.maxLod = 0.0F;
  samplerInfo.mipLodBias = 0.0F;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  const VkResult res =
      vkCreateSampler(device, &samplerInfo, nullptr, &outSampler);
  if (res != VK_SUCCESS) {
    std::cerr << "[Texture] vkCreateSampler failed: " << res << "\n";
    outSampler = VK_NULL_HANDLE;
    return false;
  }

  return true;
}
