#include "vk_texture_uploader.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/gpu/textures/vk_texture_utils.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkTextureUploader::init(VkBackendCtx &ctx) {
  m_ctx = &ctx;

  return true;
}

void VkTextureUploader::shutdown() noexcept { m_ctx = nullptr; }

bool VkTextureUploader::uploadRGBA8(VkUploadContext::Recorder recorder,
                                    const void *rgbaPixels, uint32_t width,
                                    uint32_t height, VkTexture2D &out,
                                    VkPipelineStageFlags finalStage) {
  if (!recorder) {
    std::cerr << "[TextureUpload] Invalid recorder\n";
    return false;
  }

  if (rgbaPixels == nullptr || width == 0 || height == 0) {
    std::cerr << "[TextureUpload] Invalid pixels/size\n";
    return false;
  }

  VkDevice device = m_ctx->device();

  const VkDeviceSize size = VkDeviceSize(width) * VkDeviceSize(height) * 4ULL;

  VkStagingAlloc stageAlloc = recorder.allocStaging(size, /*alignment=*/16);
  if (!stageAlloc) {
    std::cerr
        << "[TextureUpload] Out of staging space (increase per-frame budget "
           "or flush earlier)\n";
    return false;
  }

  std::memcpy(stageAlloc.ptr, rgbaPixels, static_cast<size_t>(size));

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::UploadMemcpyCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::UploadMemcpyBytes, size);

  out.shutdown();

  // TODO: check for VK_FORMAT_R8G8B8A8_UNORM
  if (!out.image.init2D(
          m_ctx->allocator(), width, height, VK_FORMAT_R8G8B8A8_SRGB,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
          VK_IMAGE_TILING_OPTIMAL)) {
    std::cerr << "[TextureUpload] Failed to create device-local image\n";
    return false;
  }

  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::TextureAllocatedBytes, size);

  recorder.cmdUploadRGBA8ToImage(
      out.image.handle(), width, height, stageAlloc.offset,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, finalStage);

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::TextureUploadCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::TextureUploadBytes, size);

  out.device = device;

  if (!vkCreateTextureView(device, out.image.handle(), VK_FORMAT_R8G8B8A8_SRGB,
                           out.view)) {
    out.shutdown();
    return false;
  }

  if (!vkCreateTextureSampler(device, out.sampler)) {
    out.shutdown();
    return false;
  }

  return true;
}
