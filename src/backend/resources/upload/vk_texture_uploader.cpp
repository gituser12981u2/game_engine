#include "vk_texture_uploader.hpp"

#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/buffers/vk_buffer.hpp"
#include "backend/resources/textures/vk_texture.hpp"
#include "backend/resources/textures/vk_texture_utils.hpp"

#include <cstdint>
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

bool VkTextureUploader::init(VmaAllocator allocator, VkDevice device,
                             VkQueue queue, VkCommands *commands,
                             UploadProfiler *profiler) {
  if (allocator == nullptr || device == VK_NULL_HANDLE ||
      queue == VK_NULL_HANDLE || commands == nullptr) {
    std::cerr << "[TextureUpload] Invalid init args\n";
    return false;
  }

  m_allocator = allocator;
  m_device = device;
  m_queue = queue;
  m_commands = commands;
  m_profiler = profiler;

  return true;
}

void VkTextureUploader::shutdown() noexcept {
  m_allocator = nullptr;
  m_device = VK_NULL_HANDLE;
  m_queue = VK_NULL_HANDLE;
  m_commands = nullptr;
  m_profiler = nullptr;
}

static VkImageMemoryBarrier makeImageBarrier(VkImage image,
                                             VkImageLayout oldLayout,
                                             VkImageLayout newLayout,
                                             VkAccessFlags srcAccess,
                                             VkAccessFlags dstAccess) {
  VkImageMemoryBarrier imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.oldLayout = oldLayout;
  imageBarrier.newLayout = newLayout;
  imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.image = image;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;
  imageBarrier.srcAccessMask = srcAccess;
  imageBarrier.dstAccessMask = dstAccess;

  return imageBarrier;
}

void VkTextureUploader::cmdTransitionImage(VkCommandBuffer cmd, VkImage image,
                                           VkImageLayout oldLayout,
                                           VkImageLayout newLayout) {
  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkAccessFlags srcAccess = 0;
  VkAccessFlags dstAccess = 0;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    srcAccess = 0;
    dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstAccess = VK_ACCESS_SHADER_READ_BIT;
  } else {
    std::cerr << "[TextureUpload] Unsupported layout transition " << oldLayout
              << " -> " << newLayout << "\n";
    return;
  }

  VkImageMemoryBarrier barrier =
      makeImageBarrier(image, oldLayout, newLayout, srcAccess, dstAccess);
  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}

bool VkTextureUploader::uploadRGBA8(const void *rgbaPixels, uint32_t width,
                                    uint32_t height, VkTexture2D &out) {
  if (m_allocator == nullptr || m_queue == VK_NULL_HANDLE ||
      m_commands == nullptr) {
    std::cerr << "[TextureUpload] Not initialized\n";
    return false;
  }

  if (rgbaPixels == nullptr || width == 0 || height == 0) {
    std::cerr << "[TextureUpload] Invalid pixels/size\n";
    return false;
  }

  const VkDeviceSize byteSize = static_cast<VkDeviceSize>(width) *
                                static_cast<VkDeviceSize>(height) * 4ULL;

  // Staging buffer: CPU-visible
  // TODO: persist the staging buffer instead of remaking it per upload
  VkBufferObj staging{};
  if (!staging.init(m_allocator, byteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VkBufferObj::MemUsage::CpuToGpu)) {
    std::cerr << "[TextureUpload] Failed to create staging buffer\n";
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::StagingCreatedCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::StagingCreatedBytes,
                byteSize);
  }

  if (!staging.upload(rgbaPixels, byteSize)) {
    std::cerr << "[TextureUpload] Failed to uploader staging data\n";
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyBytes, byteSize);
  }

  out.shutdown();
  // TODO: check for VK_FORMAT_R8G8B8A8_UNORM
  if (!out.image.init2D(m_allocator, width, height, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_TILING_OPTIMAL)) {
    std::cerr << "[TextureUpload] Failed to create device-local image\n";
    return false;
  }

  // TODO: check for transfer queue in queue family and use it
  // TODO: use timeline semaphore values to know when upload is complete
  // instead of offloading submits to a different command buffer
  const bool ok =
      m_commands->submitImmediate(m_queue, [&](VkCommandBuffer cmd) {
        cmdTransitionImage(cmd, out.image.handle(), VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;   // tightly packed
        region.bufferImageHeight = 0; // tightly packed

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = VkOffset3D{0, 0, 0};
        region.imageExtent = VkExtent3D{width, height, 1U};

        vkCmdCopyBufferToImage(cmd, staging.handle(), out.image.handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        if (m_profiler != nullptr) {
          profilerAdd(m_profiler, UploadProfiler::Stat::TextureUploadCount, 1);
          profilerAdd(m_profiler, UploadProfiler::Stat::TextureUploadBytes,
                      byteSize);
        }

        cmdTransitionImage(cmd, out.image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      });

  if (!ok) {
    std::cerr << "[TextureUpload] submitImmediate failed\n";
    out.shutdown();
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadSubmitCount, 1);
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
