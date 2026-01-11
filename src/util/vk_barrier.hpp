#pragma once

#include <vulkan/vulkan_core.h>

namespace util {

inline void cmdImageBarrier(VkCommandBuffer cmd, VkImage image,
                            VkImageLayout oldLayout, VkImageLayout newLayout,
                            VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                            VkPipelineStageFlags srcStage,
                            VkPipelineStageFlags dstStage,
                            VkImageAspectFlags aspectMask, uint32_t baseMip = 0,
                            uint32_t levelCount = 1, uint32_t baseLayer = 0,
                            uint32_t layerCount = 1) {
  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = srcAccess,
      .dstAccessMask = dstAccess,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          VkImageSubresourceRange{
              .aspectMask = aspectMask,
              .baseMipLevel = baseMip,
              .levelCount = levelCount,
              .baseArrayLayer = baseLayer,
              .layerCount = layerCount,
          },
  };

  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}

inline void cmdBufferBarrier(VkCommandBuffer cmd, VkBuffer buffer,
                             VkDeviceSize offset, VkDeviceSize size,
                             VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                             VkPipelineStageFlags srcStage,
                             VkPipelineStageFlags dstStage) {
  VkBufferMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = srcAccess,
      .dstAccessMask = dstAccess,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = buffer,
      .offset = offset,
      .size = size,
  };

  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0,
                       nullptr);
}

} // namespace util
