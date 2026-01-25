#include "vk_buffer_uploader.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/logging/log.hpp"

#include <cstddef>
#include <cstring>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkBufferUploader::init(VmaAllocator allocator) {
  m_allocator = allocator;

  return true;
}

void VkBufferUploader::shutdown() noexcept { m_allocator = nullptr; }

bool VkBufferUploader::uploadToDeviceLocalBuffer(
    VkUploadContext::Recorder recorder, const void *data, VkDeviceSize size,
    VkBufferUsageFlags finalUsage, VkBufferObj &outBuffer) {
  if (!recorder) {
    LOGE("Recorder is invalid");
    return false;
  }

  if (data == nullptr || size == 0) {
    LOGE("Data or size is invalid");
    return false;
  }

  VkStagingAlloc stageAlloc = recorder.allocStaging(size);
  if (!stageAlloc) {
    LOGE("Out of staging space");
    return false;
  }

  std::memcpy(stageAlloc.ptr, data, static_cast<size_t>(size));

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::UploadMemcpyCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::UploadMemcpyBytes, size);

  // Device-local buffer
  outBuffer.shutdown();
  if (!outBuffer.init(m_allocator, size,
                      finalUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VkBufferObj::MemUsage::GpuOnly)) {
    LOGE("Device-local buffer creation failed");
    return false;
  }

  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::BufferAllocatedBytes, size);

  recorder.cmdCopyToBuffer(outBuffer.handle(), /*dstOffset=*/0,
                           /*srcOffset=*/stageAlloc.offset, size);

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::BufferUploadCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::BufferUploadCount, size);

  return true;
}
