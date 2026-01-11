#pragma once

#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"

#include <cstdint>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class UploadProfiler;

class VkTextureUploader {
public:
  bool init(VmaAllocator allocator, VkDevice device, UploadProfiler *profiler);
  void shutdown() noexcept;

  bool uploadRGBA8(
      VkUploadContext::Recorder recorder, const void *rgbaPixels,
      uint32_t width, uint32_t height, VkTexture2D &out,
      VkPipelineStageFlags finalStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

private:
  VmaAllocator m_allocator = nullptr;   // non-owning
  VkDevice m_device = VK_NULL_HANDLE;   // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning
};
