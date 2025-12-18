#pragma once

#include "../frame/vk_commands.hpp"
#include "vk_buffer.hpp"

#include <vulkan/vulkan_core.h>

class VkUploader {
public:
  VkUploader() = default;

  bool init(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue,
            VkCommands *commands);
  void shutdown() noexcept;

  bool uploadToDeviceLocalBuffer(const void *data, VkDeviceSize size,
                                 VkBufferUsageFlags finalUsage,
                                 VkBufferObj &outBuffer);

private:
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; // non-owning
  VkDevice m_device = VK_NULL_HANDLE;                 // non-owning
  VkQueue m_queue = VK_NULL_HANDLE;                   // non-owning
  VkCommands *m_commands = nullptr;                   // non-owning
};
