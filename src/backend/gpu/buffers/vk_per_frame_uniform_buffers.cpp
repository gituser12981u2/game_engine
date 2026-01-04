#include "vk_per_frame_uniform_buffers.hpp"

#include <cstdint>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkPerFrameUniformBuffers::init(VmaAllocator allocator,
                                    uint32_t framesInFlight,
                                    VkDeviceSize strideBytes) {
  if (allocator == nullptr || framesInFlight == 0 || strideBytes == 0) {
    std::cerr << "[PerFrameUBOBufs] Invalid init args\n";
    return false;
  }

  shutdown();

  m_allocator = allocator;
  m_stride = strideBytes;

  // Buffers
  m_bufs.resize(framesInFlight);
  for (uint32_t i = 0; i < framesInFlight; ++i) {
    if (!m_bufs[i].init(allocator, m_stride, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VkBufferObj::MemUsage::CpuToGpu, /*mapped=*/true)) {
      std::cerr << "[PerFrameUBOBufs] Failed to create UBO\n";
      shutdown();
      return false;
    }
  }

  return true;
}

void VkPerFrameUniformBuffers::shutdown() noexcept {
  for (auto &b : m_bufs) {
    b.shutdown();
  }

  m_bufs.clear();
  m_stride = 0;
  m_allocator = nullptr;
}

bool VkPerFrameUniformBuffers::update(uint32_t frameIndex, const void *data,
                                      VkDeviceSize size) {
  if (frameIndex >= m_bufs.size() || data == nullptr || size > m_stride) {
    std::cerr << "[PerFrameUBO] Update invalid args\n";
    return false;
  }

  return m_bufs[frameIndex].upload(data, size);
}
