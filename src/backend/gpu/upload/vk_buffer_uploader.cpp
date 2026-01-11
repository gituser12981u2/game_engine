#include "vk_buffer_uploader.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkBufferUploader::init(VmaAllocator allocator, UploadProfiler *profiler) {
  m_allocator = allocator;
  m_profiler = profiler;

  return true;
}

void VkBufferUploader::shutdown() noexcept {
  m_allocator = nullptr;
  m_profiler = nullptr;
}

bool VkBufferUploader::uploadToDeviceLocalBuffer(
    VkUploadContext::Recorder recorder, const void *data, VkDeviceSize size,
    VkBufferUsageFlags finalUsage, VkBufferObj &outBuffer) {
  if (!recorder) {
    std::cerr << "[BufferUploader] Invalid recorder\n";
    return false;
  }

  if (data == nullptr || size == 0) {
    std::cerr << "[Uploader] Invalid data or size\n";
    return false;
  }

  VkStagingAlloc stageAlloc = recorder.allocStaging(size);
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

  recorder.cmdCopyToBuffer(outBuffer.handle(), /*dstOffset=*/0,
                           /*srcOffset=*/stageAlloc.offset, size);

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::BufferUploadCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::BufferUploadBytes, size);
  }

  return true;
}
