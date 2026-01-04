#include "vk_texture_uploader.hpp"

#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/textures/vk_texture.hpp"
#include "backend/resources/textures/vk_texture_utils.hpp"
#include "backend/resources/upload/vk_upload_context.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

// TODO take in backend ctx
bool VkTextureUploader::init(VmaAllocator allocator, VkDevice device,
                             VkUploadContext *upload,
                             UploadProfiler *profiler) {
  if (allocator == nullptr || device == VK_NULL_HANDLE ||
      upload == VK_NULL_HANDLE) {
    std::cerr << "[TextureUpload] Invalid init args\n";
    return false;
  }

  m_allocator = allocator;
  m_device = device;
  m_upload = upload;
  m_profiler = profiler;

  return true;
}

void VkTextureUploader::shutdown() noexcept {
  m_allocator = nullptr;
  m_device = VK_NULL_HANDLE;
  m_upload = nullptr;
  m_profiler = nullptr;
}

bool VkTextureUploader::uploadRGBA8(const void *rgbaPixels, uint32_t width,
                                    uint32_t height, VkTexture2D &out) {
  if (m_allocator == nullptr || m_upload == nullptr) {
    std::cerr << "[TextureUpload] Not initialized\n";
    return false;
  }

  if (rgbaPixels == nullptr || width == 0 || height == 0) {
    std::cerr << "[TextureUpload] Invalid pixels/size\n";
    return false;
  }

  const VkDeviceSize size = static_cast<VkDeviceSize>(width) *
                            static_cast<VkDeviceSize>(height) * 4ULL;

  VkStagingAlloc stageAlloc = m_upload->allocStaging(size, /*alignment=*/16);
  if (!stageAlloc) {
    std::cerr
        << "[TextureUpload] Out of staging space (increase per-frame budget "
           "or flush earlier)\n";
    return false;
  }

  std::memcpy(stageAlloc.ptr, rgbaPixels, static_cast<size_t>(size));

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyBytes, size);
  }

  out.shutdown();

  // TODO: check for VK_FORMAT_R8G8B8A8_UNORM
  // TODO: move to images/
  if (!out.image.init2D(m_allocator, width, height, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_TILING_OPTIMAL)) {
    std::cerr << "[TextureUpload] Failed to create device-local image\n";
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::TextureAllocatedBytes, size);
  }

  m_upload->cmdUploadRGBA8ToImage(out.image.handle(), width, height,
                                  stageAlloc.offset,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::TextureUploadCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::TextureUploadBytes, size);
  }

  out.device = m_device;

  if (!vkCreateTextureView(m_device, out.image.handle(),
                           VK_FORMAT_R8G8B8A8_SRGB, out.view)) {
    out.shutdown();
    return false;
  }

  if (!vkCreateTextureSampler(m_device, out.sampler)) {
    out.shutdown();
    return false;
  }

  return true;
}
