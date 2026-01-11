#pragma once

#include "backend/gpu/buffers/vk_buffer.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class UploadProfiler;

class VkBufferUploader {
public:
  VkBufferUploader() = default;

  bool init(VmaAllocator allocator, UploadProfiler *profiler);
  void shutdown() noexcept;

  bool uploadToDeviceLocalBuffer(VkUploadContext::Recorder recorder,
                                 const void *data, VkDeviceSize size,
                                 VkBufferUsageFlags finalUsage,
                                 VkBufferObj &outBuffer);

private:
  VmaAllocator m_allocator = nullptr;   // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning
};
