#include "vk_buffer_uploader.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkBufferUploader::init(VmaAllocator allocator, VkUploadContext *upload,
                            UploadProfiler *profiler) {
  if (allocator == nullptr || upload == nullptr) {
    std::cerr << "[Uploader] Invalid init args\n";
    return false;
  }

  m_allocator = allocator;
  m_upload = upload;
  m_profiler = profiler;

  return true;
}

void VkBufferUploader::shutdown() noexcept {
  m_allocator = nullptr;
  m_upload = nullptr;
  m_profiler = nullptr;
}

bool VkBufferUploader::uploadToDeviceLocalBuffer(const void *data,
                                                 VkDeviceSize size,
                                                 VkBufferUsageFlags finalUsage,
                                                 VkBufferObj &outBuffer) {
  if (m_allocator == nullptr || m_upload == nullptr) {
    std::cerr << "[Uploader] Not initialized\n";
    return false;
  }

  if (data == nullptr || size == 0) {
    std::cerr << "[Uploader] Invalid data or size\n";
    return false;
  }

  VkStagingAlloc stageAlloc = m_upload->allocStaging(size);
  if (!stageAlloc) {
    std::cerr << "[Uploader] Out of staging space (increase per-frame budget "
                 "or flush earlier)\n";
    return false;
  }

  std::memcpy(stageAlloc.ptr, data, static_cast<size_t>(size));

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyBytes, size);
  }

  // Device-local buffer
  outBuffer.shutdown();
  if (!outBuffer.init(m_allocator, size,
                      finalUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VkBufferObj::MemUsage::GpuOnly)) {
    std::cerr << "[Uploader] Failed to create device-local buffer\n";
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::BufferAllocatedBytes, size);
  }

  m_upload->cmdCopyToBuffer(outBuffer.handle(), /*dstOffset=*/0,
                            /*srcOffset=*/stageAlloc.offset, size);

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::BufferUploadCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::BufferUploadBytes, size);
  }

  return true;
}
