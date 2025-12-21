#include "vk_uploader.hpp"
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkUploader::init(VkPhysicalDevice physicalDevice, VkDevice device,
                      VkQueue queue, VkCommands *commands) {
  if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE ||
      queue == VK_NULL_HANDLE || commands == nullptr) {
    std::cerr << "[Uploader] Invalid init args\n";
    return false;
  }

  m_physicalDevice = physicalDevice;
  m_device = device;
  m_queue = queue;
  m_commands = commands;

  return true;
}

void VkUploader::shutdown() noexcept {
  m_physicalDevice = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
  m_queue = VK_NULL_HANDLE;
  m_commands = nullptr;
}

bool VkUploader::uploadToDeviceLocalBuffer(const void *data, VkDeviceSize size,
                                           VkBufferUsageFlags finalUsage,
                                           VkBufferObj &outBuffer) {
  if (m_device == VK_NULL_HANDLE || m_physicalDevice == VK_NULL_HANDLE ||
      m_queue == VK_NULL_HANDLE || m_commands == nullptr) {
    std::cerr << "[Uploader] Not initialized\n";
    return false;
  }

  if (data == nullptr || size == 0) {
    std::cerr << "[Uploader] Invalid data or size\n";
    return false;
  }

  // Staging buffer: CPU-visible
  // TODO: persist the staging buffer instead of remaking it per upload
  VkBufferObj staging{};
  if (!staging.init(m_physicalDevice, m_device, size,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    std::cerr << "[Uploader] Failed to create staging buffer\n";
    return false;
  }

  if (!staging.upload(data, size)) {
    std::cerr << "[Uploader] Failed to upload staging data\n";
    return false;
  }

  outBuffer.shutdown();
  if (!outBuffer.init(m_physicalDevice, m_device, size,
                      finalUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
    std::cerr << "[Uploader] Failed to create index buffer\n";
    return false;
  }

  // TODO: check for transfer queue in queue family and use it
  // TODO: use timeline semaphore values to know when upload is complete
  // instead of offloading submits to a different command buffer
  const bool ok =
      m_commands->submitImmediate(m_queue, [&](VkCommandBuffer cmd) {
        VkBufferCopy region{};
        region.size = size;
        vkCmdCopyBuffer(cmd, staging.handle(), outBuffer.handle(), 1, &region);
      });

  if (!ok) {
    std::cerr << "[Uploader] submitImmediate copy failed\n";
    outBuffer.shutdown();
    return false;
  }

  return true;
}
