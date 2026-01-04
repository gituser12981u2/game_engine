#pragma once

#include "backend/gpu/textures/vk_texture.hpp"

class VkUploadContext;
class VkCommands;
class UploadProfiler;

#include <cstdint>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkTextureUploader {
public:
  bool init(VmaAllocator allocator, VkDevice device, VkUploadContext *upload,
            UploadProfiler *profiler);
  void shutdown() noexcept;

  bool uploadRGBA8(const void *rgbaPixels, uint32_t width, uint32_t height,
                   VkTexture2D &out);

private:
  VmaAllocator m_allocator = nullptr; // non-owning
  VkDevice m_device = VK_NULL_HANDLE; // non-owning

  VkUploadContext *m_upload = nullptr;  // non-owning
  VkCommands *m_commands = nullptr;     // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning
};
