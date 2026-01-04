#include "backend/resources/upload/vk_upload_context.hpp"
#include "backend/core/vk_backend_ctx.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/buffers/vk_buffer.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan_core.h>

template <typename T> static inline void freeArray(T *&p) noexcept {
  std::free(static_cast<void *>(p));
  p = nullptr;
}

VkDeviceSize VkUploadContext::alignUp(VkDeviceSize v, VkDeviceSize a) noexcept {
  if (a == 0) {
    return v;
  }

  return (v + (a - 1)) & ~(a - 1);
}

bool VkUploadContext::init(VkBackendCtx &ctx, uint32_t framesInflight,
                           VkDeviceSize perFrameBytes,
                           UploadProfiler *profiler) {
  if (perFrameBytes == 0) {
    std::cerr << "[UploadCtx] Invalid init args\n";
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_framesInFlight = framesInflight;
  m_perFrameBytes = perFrameBytes;
  m_profiler = profiler;

  // Query device limits for alignment
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(m_ctx->physicalDevice(), &props);
  m_bufCopyAlign =
      std::max<VkDeviceSize>(1, props.limits.optimalBufferCopyOffsetAlignment);
  m_rowPitchAlign = std::max<VkDeviceSize>(
      1, props.limits.optimalBufferCopyRowPitchAlignment);

  const VkDeviceSize totalBytes = VkDeviceSize(framesInflight) * perFrameBytes;

  // TODO: create per thread
  // Create per frame
  m_pools = (VkCommandPool *)std::calloc(framesInflight, sizeof(VkCommandPool));
  m_cmds =
      (VkCommandBuffer *)std::calloc(framesInflight, sizeof(VkCommandBuffer));
  m_fences = (VkFence *)std::calloc(framesInflight, sizeof(VkFence));

  if (!m_staging.init(m_ctx->allocator(), totalBytes,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VkBufferObj::MemUsage::CpuToGpu, /*mapped*/ true)) {
    std::cerr << "[UploadCtx] Failed to create staging buffer\n";
    shutdown();
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::StagingCreatedCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::StagingAllocatedBytes,
                totalBytes);
  }

  {
    void *mapped = nullptr;
    VkResult res =
        vmaMapMemory(m_ctx->allocator(), m_staging.allocation(), &mapped);
    if (res != VK_SUCCESS || mapped == nullptr) {
      std::cerr << "[UploadCtx] vmaMapMemory staging failed: " << res << "\n";
      shutdown();
      return false;
    }

    m_stagingMapped = mapped;
  }

  for (uint32_t i = 0; i < framesInflight; ++i) {
    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = m_ctx->graphicsQueueFamily();

    VkResult res = vkCreateCommandPool(m_ctx->device(), &cmdPoolInfo, nullptr,
                                       &m_pools[i]);
    if (res != VK_SUCCESS) {
      std::cerr << "[UploadCtx] vkCreateComandPool failed: " << res << "\n";
      shutdown();
      return false;
    }

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = m_pools[i];
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    res = vkAllocateCommandBuffers(m_ctx->device(), &cmdAllocInfo, &m_cmds[i]);
    if (res != VK_SUCCESS) {
      std::cerr << "[UploadCtx] vkAllocateCommandBuffers failed: " << res
                << "\n";
      shutdown();
      return false;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    res = vkCreateFence(m_ctx->device(), &fenceInfo, nullptr, &m_fences[i]);
    if (res != VK_SUCCESS) {
      std::cerr << "[UploadCtx] vkCreateFence failed: " << res << "\n";
      shutdown();
      return false;
    }
  }

  return true;
}

void VkUploadContext::shutdown() noexcept {
  VkDevice device = VK_NULL_HANDLE;
  if (m_ctx != nullptr) {
    device = m_ctx->device();
  }

  if (device != VK_NULL_HANDLE) {
    if (m_fences != nullptr) {
      for (uint32_t i = 0; i < m_framesInFlight; ++i) {
        if (m_fences[i] != VK_NULL_HANDLE) {
          vkDestroyFence(device, m_fences[i], nullptr);
          m_fences[i] = VK_NULL_HANDLE;
        }
      }
    }

    if (m_cmds != nullptr && m_pools != nullptr) {
      for (uint32_t i = 0; i < m_framesInFlight; ++i) {
        if (m_cmds[i] != VK_NULL_HANDLE && m_pools[i] != VK_NULL_HANDLE) {
          vkFreeCommandBuffers(device, m_pools[i], 1, &m_cmds[i]);
          m_cmds[i] = VK_NULL_HANDLE;
        }
      }
    }

    if (m_pools != nullptr) {
      for (uint32_t i = 0; i < m_framesInFlight; ++i) {
        if (m_pools[i] != VK_NULL_HANDLE) {
          vkDestroyCommandPool(device, m_pools[i], nullptr);
          m_pools[i] = VK_NULL_HANDLE;
        }
      }
    }
  }

  if (m_ctx != nullptr && m_stagingMapped != nullptr) {
    vmaUnmapMemory(m_ctx->allocator(), m_staging.allocation());
  }

  freeArray(m_fences);
  freeArray(m_cmds);
  freeArray(m_pools);

  m_fences = nullptr;
  m_cmds = nullptr;
  m_pools = nullptr;

  m_pool = VK_NULL_HANDLE;
  m_cmd = VK_NULL_HANDLE;

  m_stagingMapped = nullptr;
  m_staging.shutdown();

  m_ctx = nullptr;
  m_profiler = nullptr;

  m_framesInFlight = 0;
  m_frameIndex = 0;
  m_perFrameBytes = 0;

  m_bufCopyAlign = 1;
  m_rowPitchAlign = 1;

  m_sliceBase = 0;
  m_sliceHead = 0;
  m_recording = false;
}

bool VkUploadContext::beginCmd() {
  // TODO: make single method for this, renderer, and command to use
  VkCommandBufferBeginInfo bufBeginInfo{};
  bufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VkResult res = vkBeginCommandBuffer(m_cmd, &bufBeginInfo);
  if (res != VK_SUCCESS) {
    std::cerr << "[UploadCtx] vkBeginCommandBuffer failed: " << res << "\n";
    return false;
  }

  return true;
}

bool VkUploadContext::endCmd() {
  VkResult res = vkEndCommandBuffer(m_cmd);
  if (res != VK_SUCCESS) {
    std::cerr << "[UploadCtx] vkEndCommandBuffer failed: " << res << "\n";
    return false;
  }

  return true;
}

bool VkUploadContext::beginFrame(uint32_t frameIndex) {
  if (m_ctx == nullptr || m_pools == VK_NULL_HANDLE || m_fences == nullptr) {
    std::cerr << "[UploadCtx] beginFrame invalid state\n";
    return false;
  }

  if (frameIndex >= m_framesInFlight) {
    std::cerr << "[UploadCtx] beginFrame frameIndex out of range\n";
    return false;
  }

  m_frameIndex = frameIndex;
  m_pool = m_pools[frameIndex];
  m_cmd = m_cmds[frameIndex];

  VkFence fence = m_fences[frameIndex];

  // TODO: use timeline semaphore to not blocking wait
  VkResult res =
      vkWaitForFences(m_ctx->device(), 1, &fence, VK_TRUE, UINT64_MAX);
  if (res != VK_SUCCESS) {
    std::cerr << "[UploadCtx] vkWaitForFences failed: " << res << "\n";
    return false;
  }

  vkResetCommandPool(m_ctx->device(), m_pools[frameIndex], 0);

  m_sliceBase = VkDeviceSize(frameIndex) * m_perFrameBytes;
  m_sliceHead = 0;

  if (!beginCmd()) {
    return false;
  }

  m_recording = true;
  m_hadWork = false;
  return true;
}

VkStagingAlloc VkUploadContext::allocStaging(VkDeviceSize size,
                                             VkDeviceSize alignment) {
  VkStagingAlloc out{};

  if (!m_recording || m_stagingMapped == nullptr) {
    return out;
  }

  VkDeviceSize a = std::max(alignment, m_bufCopyAlign);
  VkDeviceSize alignedHead = alignUp(m_sliceHead, a);

  if (alignedHead + size > m_perFrameBytes) {
    return out;
  }

  VkDeviceSize absOffset = m_sliceBase + alignedHead;
  out.ptr = static_cast<std::uint8_t *>(m_stagingMapped) + absOffset;
  out.offset = absOffset;
  out.size = size;

  m_sliceHead = alignedHead + size;
  m_hadWork = true;

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::StagingUsedBytes, size);
  }

  return out;
}

void VkUploadContext::cmdCopyToBuffer(VkBuffer dst, VkDeviceSize dstOffset,
                                      VkDeviceSize srcOffset,
                                      VkDeviceSize size) {
  if (!m_recording) {
    return;
  }

  m_hadWork = true;

  VkBufferCopy copy{};
  copy.srcOffset = srcOffset;
  copy.dstOffset = dstOffset;
  copy.size = size;
  vkCmdCopyBuffer(m_cmd, m_staging.handle(), dst, 1, &copy);
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

// TODO: generalize to include frag and vertex to cut down on copy
// code
void VkUploadContext::cmdBarrierBufferTransferToVertexShader(
    VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
  if (!m_recording) {
    return;
  }

  VkBufferMemoryBarrier bufBarrier{};
  bufBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  bufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufBarrier.buffer = buffer;
  bufBarrier.offset = offset;
  bufBarrier.size = size;

  vkCmdPipelineBarrier(m_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1,
                       &bufBarrier, 0, nullptr);
}

void VkUploadContext::cmdBarrierBufferTransferToFragmentShader(
    VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
  if (!m_recording) {
    return;
  }

  VkBufferMemoryBarrier bufBarrier{};
  bufBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  bufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufBarrier.buffer = buffer;
  bufBarrier.offset = offset;
  bufBarrier.size = size;

  vkCmdPipelineBarrier(m_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1,
                       &bufBarrier, 0, nullptr);
}

void VkUploadContext::transitionImage(VkImage image, VkImageLayout oldLayout,
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
    std::cerr << "[UploadCtx] Unsupported layout transition " << oldLayout
              << " -> " << newLayout << "\n";
    return;
  }

  VkImageMemoryBarrier barrier =
      makeImageBarrier(image, oldLayout, newLayout, srcAccess, dstAccess);
  vkCmdPipelineBarrier(m_cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}

void VkUploadContext::cmdUploadRGBA8ToImage(VkImage image, uint32_t width,
                                            uint32_t height,
                                            VkDeviceSize srcOffset,
                                            VkImageLayout finalLayout) {
  if (!m_recording) {
    return;
  }

  transitionImage(image, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy region{};
  region.bufferOffset = srcOffset;
  region.bufferRowLength = 0;   // tightly packed
  region.bufferImageHeight = 0; // tightly packed
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = VkOffset3D{0, 0, 0};
  region.imageExtent = VkExtent3D{width, height, 1U};

  vkCmdCopyBufferToImage(m_cmd, m_staging.handle(), image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  transitionImage(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);
}

bool VkUploadContext::flush(bool wait) {
  if (!m_recording) {
    return true;
  }

  // End the recording scope if no work done
  if (!m_hadWork) {
    if (!endCmd()) {
      return false;
    }

    m_recording = false;
    return true;
  }

  m_recording = false;

  if (!endCmd()) {
    return false;
  }

  VkFence fence = m_fences[m_frameIndex];
  VkResult res = vkResetFences(m_ctx->device(), 1, &fence);
  if (res != VK_SUCCESS) {
    std::cerr << "[UploadCtx] vkResetFences failed: " << res << "\n";
    return false;
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer cmd = m_cmds[m_frameIndex];
  submitInfo.pCommandBuffers = &cmd;

  res = vkQueueSubmit(m_ctx->graphicsQueue(), 1, &submitInfo, fence);
  if (res != VK_SUCCESS) {
    std::cerr << "[UploadCtx] vkQueueSubmit failed: " << res << "\n";
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadSubmitCount, 1);
  }

  if (wait) {
    vkWaitForFences(m_ctx->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
      std::cerr << "[UploadCtx] vkWaitForFences(wait) failed: " << res << "\n";
      return false;
    }
  }

  return true;
}
