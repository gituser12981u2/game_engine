#include "vk_buffer.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

static VmaMemoryUsage toVmaUsage(VkBufferObj::MemUsage usage) {
  switch (usage) {
  case VkBufferObj::MemUsage::GpuOnly:
    return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  case VkBufferObj::MemUsage::CpuToGpu:
  case VkBufferObj::MemUsage::GpuToCpu:
    return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
  }

  return VMA_MEMORY_USAGE_AUTO;
}

bool VkBufferObj::init(VmaAllocator allocator, VkDeviceSize size,
                       VkBufferUsageFlags usage, MemUsage memUsage,
                       bool mapped) {
  if (allocator == nullptr || size == 0) {
    std::cerr << "[Buffer] init invalid args\n";
    return false;
  }

  shutdown();

  m_allocator = allocator;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = toVmaUsage(memUsage);

  if (memUsage == MemUsage::CpuToGpu) {
    allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  } else if (memUsage == MemUsage::GpuToCpu) {
    allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
  }

  if (mapped) {
    allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  VkResult res = vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
                                 &m_buffer, &m_allocation, nullptr);
  if (res != VK_SUCCESS) {
    std::cerr << "[Buffer] vmaCreateBuffer failed: " << res << "\n";
    shutdown();
    return false;
  }

  m_size = size;
  return true;
}

bool VkBufferObj::upload(const void *data, VkDeviceSize size,
                         VkDeviceSize offset) {
  if (!valid() || data == nullptr || m_allocation == nullptr ||
      data == nullptr) {
    std::cerr << "[Buffer] upload invalids args\n";
    return false;
  }

  if (offset + size > m_size) {
    std::cerr
        << "[Buffer] offset + size is larger than the destionation size\n";
    return false;
  }

  void *mapped = nullptr;
  VkResult res = vmaMapMemory(m_allocator, m_allocation, &mapped);
  if (res != VK_SUCCESS || mapped == nullptr) {
    std::cerr << "[Buffer] vmaMapMemory failed: " << res << "\n";
    return false;
  }

  std::memcpy(static_cast<std::byte *>(mapped) + offset, data,
              static_cast<size_t>(size));
  vmaUnmapMemory(m_allocator, m_allocation);
  return true;
}

void VkBufferObj::shutdown() noexcept {
  if (m_allocator != nullptr && m_buffer != VK_NULL_HANDLE &&
      m_allocation != nullptr) {
    vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
  }

  m_buffer = VK_NULL_HANDLE;
  m_allocation = nullptr;
  m_allocator = nullptr;
  m_size = 0;
}
