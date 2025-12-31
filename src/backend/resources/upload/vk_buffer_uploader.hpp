#pragma once

#include "backend/frame/vk_commands.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/buffers/vk_buffer.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkBufferUploader {
public:
  VkBufferUploader() = default;

  bool init(VmaAllocator allocator, VkQueue queue, VkCommands *commands,
            UploadProfiler *profiler);
  void shutdown() noexcept;

  bool uploadToDeviceLocalBuffer(const void *data, VkDeviceSize size,
                                 VkBufferUsageFlags finalUsage,
                                 VkBufferObj &outBuffer);

private:
  VmaAllocator m_allocator = nullptr; // non-owning
  VkQueue m_queue = VK_NULL_HANDLE;   // non-owning
  VkCommands *m_commands = nullptr;   // non-owning
  UploadProfiler *m_profiler = nullptr;
};
