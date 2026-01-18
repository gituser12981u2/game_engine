#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"

#include <cstdint>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkTextureUploader {
public:
  bool init(VkBackendCtx &ctx);
  void shutdown() noexcept;

  bool uploadRGBA8(
      VkUploadContext::Recorder recorder, const void *rgbaPixels,
      uint32_t width, uint32_t height, VkTexture2D &out,
      VkPipelineStageFlags finalStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

private:
  VkBackendCtx *m_ctx = nullptr; // non-owning
};
