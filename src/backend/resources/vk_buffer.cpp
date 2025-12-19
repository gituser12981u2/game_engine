#include "vk_buffer.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_core.h>

static bool findMemoryTypeIndex(VkPhysicalDevice physicalDevice,
                                uint32_t typeBits, VkMemoryPropertyFlags props,
                                uint32_t &outIndex) {
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
    const bool typeOk = (typeBits & (1U << i)) != 0;
    const bool propsOk =
        (memoryProperties.memoryTypes[i].propertyFlags & props) == props;
    if (typeOk && propsOk) {
      outIndex = i;
      return true;
    }
  }
  return false;
}

bool VkBufferObj::init(VkPhysicalDevice physicalDevice, VkDevice device,
                       VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags memProps) {

  if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
    std::cerr << "[Buffer] init: invalid args\n";
    return false;
  }

  shutdown();
  m_device = device;

  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);
  if (res != VK_SUCCESS) {
    std::cerr << "[Buffer] vkCreateBuffer failed: " << res << "\n";
    m_buffer = VK_NULL_HANDLE;
    return false;
  }

  VkMemoryRequirements memReq{};
  vkGetBufferMemoryRequirements(device, m_buffer, &memReq);

  uint32_t memIndex = 0;
  if (!findMemoryTypeIndex(physicalDevice, memReq.memoryTypeBits, memProps,
                           memIndex)) {
    std::cerr << "[Buffer] No suitable memory type\n";
    shutdown();
    return false;
  }

  VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocateInfo.allocationSize = memReq.size;
  allocateInfo.memoryTypeIndex = memIndex;

  res = vkAllocateMemory(device, &allocateInfo, nullptr, &m_memory);
  if (res != VK_SUCCESS) {
    std::cerr << "[Buffer] vkAllocateMemory failed: " << res << "\n";
    shutdown();
    return false;
  }

  res = vkBindBufferMemory(device, m_buffer, m_memory, 0);
  if (res != VK_SUCCESS) {
    std::cerr << "[Buffer] vkBindBufferMemory failed: " << res << "\n";
    shutdown();
    return false;
  }

  m_size = size;
  return true;
}

bool VkBufferObj::upload(const void *data, VkDeviceSize size,
                         VkDeviceSize offset) {
  if (!valid() || data == nullptr || m_memory == VK_NULL_HANDLE ||
      data == nullptr) {
    std::cerr << "[Buffer] upload: invalids args\n";
    return false;
  }

  if (offset + size > m_size) {
    std::cerr
        << "[Buffer] offset + size is larger than the destionation size\n";
    return false;
  }

  void *mapped = nullptr;
  VkResult res = vkMapMemory(m_device, m_memory, offset, size, 0, &mapped);
  if (res != VK_SUCCESS) {
    std::cerr << "[Buffer] vkMapMemory failed: " << res << "\n";
    return false;
  }

  std::memcpy(mapped, data, static_cast<size_t>(size));
  vkUnmapMemory(m_device, m_memory);
  return true;
}

void VkBufferObj::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    if (m_buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(m_device, m_buffer, nullptr);
    }

    if (m_memory != VK_NULL_HANDLE) {
      vkFreeMemory(m_device, m_memory, nullptr);
    }
  }

  m_buffer = VK_NULL_HANDLE;
  m_memory = VK_NULL_HANDLE;
  m_size = 0;
  m_device = VK_NULL_HANDLE;
}
