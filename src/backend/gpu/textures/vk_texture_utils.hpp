#pragma once

#include <vulkan/vulkan_core.h>

bool vkCreateTextureView(VkDevice device, VkImage image, VkFormat format,
                         VkImageView &outView);
bool vkCreateTextureSampler(VkDevice device, VkSampler &outSampler);
