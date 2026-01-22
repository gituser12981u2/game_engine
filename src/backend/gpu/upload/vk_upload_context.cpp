#include "backend/gpu/upload/vk_upload_context.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/buffers/vk_buffer.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/logging/log.hpp"
#include "util/vk_barrier.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

template <typename T> static inline void freeArray(T *&p) noexcept {
  std::free(static_cast<void *>(p));
  p = nullptr;
}

VkUploadContext &VkUploadContext::operator=(VkUploadContext &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  shutdown();

  m_ctx = std::exchange(other.m_ctx, nullptr);
  m_mode = std::exchange(other.m_mode, Mode::FrameRing);

  m_framesInFlight = std::exchange(other.m_framesInFlight, 0);
  m_threadCount = std::exchange(other.m_threadCount, 0);
  m_bytesPerSlice = std::exchange(other.m_bytesPerSlice, 0);

  m_bufCopyAlign = std::exchange(other.m_bufCopyAlign, 1);
  m_rowPitchAlign = std::exchange(other.m_rowPitchAlign, 1);

  m_staging = std::move(other.m_staging);
  m_stagingMapped = std::exchange(other.m_stagingMapped, nullptr);

  m_pools = std::exchange(other.m_pools, nullptr);
  m_cmds = std::exchange(other.m_cmds, nullptr);
  m_fences = std::exchange(other.m_fences, nullptr);
  m_heads = std::exchange(other.m_heads, nullptr);
  m_begun = std::exchange(other.m_begun, nullptr);
  m_hadWork = std::exchange(other.m_hadWork, nullptr);

  m_submitScratch = std::move(other.m_submitScratch);

  return *this;
}

VkDeviceSize VkUploadContext::alignUp(VkDeviceSize v, VkDeviceSize a) noexcept {
  if (a == 0) {
    return v;
  }

  return (v + (a - 1)) & ~(a - 1);
}

VkCommandPool VkUploadContext::poolAt(uint32_t frameIndex,
                                      uint32_t threadIndex) const noexcept {
  if (m_pools == nullptr) {
    return VK_NULL_HANDLE;
  }

  return m_pools[idx(frameIndex, threadIndex)];
}

VkCommandBuffer VkUploadContext::cmdAt(uint32_t frameIndex,
                                       uint32_t threadIndex) const noexcept {
  if (m_cmds == nullptr) {
    return VK_NULL_HANDLE;
  }

  return m_cmds[idx(frameIndex, threadIndex)];
}

bool VkUploadContext::initCommon(VkBackendCtx &ctx, Mode mode,
                                 uint32_t framesInflight,
                                 VkDeviceSize bytesPerFrameSlice,
                                 uint32_t threadCount) {
  shutdown();

  m_ctx = &ctx;
  m_mode = mode;

  m_framesInFlight = framesInflight;
  m_threadCount = threadCount;
  m_bytesPerSlice = bytesPerFrameSlice;

  // Query device limits for alignment
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(m_ctx->physicalDevice(), &props);
  m_bufCopyAlign =
      std::max<VkDeviceSize>(1, props.limits.optimalBufferCopyOffsetAlignment);
  m_rowPitchAlign = std::max<VkDeviceSize>(
      1, props.limits.optimalBufferCopyRowPitchAlignment);

  const VkDeviceSize totalBytes =
      VkDeviceSize(framesInflight) * m_bytesPerSlice;
  const uint32_t poolCount = m_framesInFlight * m_threadCount;

  m_pools = (VkCommandPool *)std::calloc(poolCount, sizeof(VkCommandPool));
  m_cmds = (VkCommandBuffer *)std::calloc(poolCount, sizeof(VkCommandBuffer));
  m_fences = (VkFence *)std::calloc(m_framesInFlight, sizeof(VkFence));

  m_heads = (std::atomic<VkDeviceSize> *)std::calloc(
      m_framesInFlight, sizeof(std::atomic<VkDeviceSize>));
  m_begun = (uint8_t *)std::calloc(poolCount, sizeof(uint8_t));
  m_hadWork = (uint8_t *)std::calloc(poolCount, sizeof(uint8_t));

  if (m_pools == nullptr || m_cmds == nullptr || m_fences == nullptr ||
      m_heads == nullptr || m_begun == nullptr || m_hadWork == nullptr) {
    LOGE("Allocation failed");
    shutdown();
    return false;
  }

  // Construct atomics
  for (uint32_t i = 0; i < m_framesInFlight; ++i) {
    new (&m_heads[i]) std::atomic<VkDeviceSize>(0);
  }

  if (!m_staging.init(m_ctx->allocator(), totalBytes,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VkBufferObj::MemUsage::CpuToGpu, /*mapped*/ true)) {
    LOGE("Staging buffer creation failed");
    shutdown();
    return false;
  }

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::StagingCreatedCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::StagingAllocatedBytes, totalBytes);

  {
    void *mapped = nullptr;
    VkResult res =
        vmaMapMemory(m_ctx->allocator(), m_staging.allocation(), &mapped);
    if (res != VK_SUCCESS || mapped == nullptr) {
      LOGE("vmaMapMemory staging failed: {}", fmt::underlying(res));
      shutdown();
      return false;
    }

    m_stagingMapped = mapped;
  }

  // Fences per frame slot
  for (uint32_t fi = 0; fi < framesInflight; ++fi) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult res =
        vkCreateFence(m_ctx->device(), &fenceInfo, nullptr, &m_fences[fi]);
    if (res != VK_SUCCESS) {
      LOGE("vkCreateFence failed: {}", fmt::underlying(res));
      shutdown();
      return false;
    }
  }

  // Create per-frame/per-thread pools and cmd buffers
  for (uint32_t fi = 0; fi < m_framesInFlight; ++fi) {
    for (uint32_t ti = 0; ti < m_threadCount; ++ti) {
      VkCommandPoolCreateInfo cmdPoolInfo{};
      cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      cmdPoolInfo.queueFamilyIndex = m_ctx->graphicsQueueFamily();

      VkCommandPool pool = VK_NULL_HANDLE;
      VkResult res =
          vkCreateCommandPool(m_ctx->device(), &cmdPoolInfo, nullptr, &pool);
      if (res != VK_SUCCESS) {
        LOGE("vkCreateCommandPool failed: {}", fmt::underlying(res));
        shutdown();
        return false;
      }
      m_pools[idx(fi, ti)] = pool;

      VkCommandBufferAllocateInfo cmdAllocInfo{};
      cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmdAllocInfo.commandPool = pool;
      cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cmdAllocInfo.commandBufferCount = 1;

      VkCommandBuffer cmd = VK_NULL_HANDLE;
      res = vkAllocateCommandBuffers(m_ctx->device(), &cmdAllocInfo, &cmd);
      if (res != VK_SUCCESS) {
        LOGE("vkAllocateCommandBuffers failed: {}", fmt::underlying(res));
        shutdown();
        return false;
      }
      m_cmds[idx(fi, ti)] = cmd;
    }
  }

  m_submitScratch.clear();
  m_submitScratch.shrink_to_fit();
  m_submitScratch.reserve(m_threadCount);

  return true;
}

bool VkUploadContext::initFrameRing(VkBackendCtx &ctx, uint32_t framesInFlight,
                                    VkDeviceSize bytesPerFrameSlice,
                                    uint32_t threadCount) {
  if (framesInFlight == 0 || bytesPerFrameSlice == 0 || threadCount == 0) {
    LOGE("Invalid arguments");
    return false;
  }

  return initCommon(ctx, Mode::FrameRing, framesInFlight, bytesPerFrameSlice,
                    threadCount);
}

bool VkUploadContext::initOneShot(VkBackendCtx &ctx, VkDeviceSize totalBytes,
                                  uint32_t threadCount) {
  if (totalBytes == 0 || threadCount == 0) {
    LOGE("Invalid arguments");
    return false;
  }

  return initCommon(ctx, Mode::OneShot, /*framesInflight=*/1, totalBytes,
                    threadCount);
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
      for (uint32_t fi = 0; fi < m_framesInFlight; ++fi) {
        for (uint32_t ti = 0; ti < m_threadCount; ++ti) {
          VkCommandBuffer cmd = m_cmds[idx(fi, ti)];
          VkCommandPool pool = m_pools[idx(fi, ti)];
          if (cmd != VK_NULL_HANDLE && pool != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, pool, 1, &cmd);
            m_cmds[idx(fi, ti)] = VK_NULL_HANDLE;
          }
        }
      }
    }

    if (m_pools != nullptr) {
      for (uint32_t fi = 0; fi < m_framesInFlight; ++fi) {
        for (uint32_t ti = 0; ti < m_threadCount; ++ti) {
          VkCommandPool pool = m_pools[idx(fi, ti)];
          if (pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, pool, nullptr);
            m_pools[idx(fi, ti)] = VK_NULL_HANDLE;
          }
        }
      }
    }
  }

  if (m_ctx != nullptr && m_stagingMapped != nullptr) {
    vmaUnmapMemory(m_ctx->allocator(), m_staging.allocation());
  }

  if (m_heads != nullptr) {
    for (uint32_t i = 0; i < m_framesInFlight; ++i) {
      m_heads[i].~atomic();
    }
  }

  freeArray(m_hadWork);
  freeArray(m_begun);
  freeArray(m_heads);
  freeArray(m_fences);
  freeArray(m_cmds);
  freeArray(m_pools);

  m_stagingMapped = nullptr;
  m_staging.shutdown();

  m_ctx = nullptr;
  m_mode = Mode::FrameRing;
  m_framesInFlight = 0;
  m_threadCount = 0;
  m_bytesPerSlice = 0;

  m_bufCopyAlign = 1;
  m_rowPitchAlign = 1;

  m_submitScratch.clear();
}

bool VkUploadContext::waitAndReset(uint32_t frameIndex) {
  if (m_ctx == nullptr || m_pools == VK_NULL_HANDLE || m_fences == nullptr) {
    LOGE("waitAndReset invalid state");
    return false;
  }

  if (frameIndex >= m_framesInFlight) {
    LOGE("waitAndReset frameIndex out of range");
    return false;
  }

  VkFence fence = m_fences[frameIndex];

  // TODO: use timeline semaphore to not blocking wait
  VkResult res =
      vkWaitForFences(m_ctx->device(), 1, &fence, VK_TRUE, UINT64_MAX);
  if (res != VK_SUCCESS) {
    LOGE("vkWaitForFences failed: {}", fmt::underlying(res));
    return false;
  }

  for (uint32_t ti = 0; ti < m_threadCount; ++ti) {
    vkResetCommandPool(m_ctx->device(), m_pools[idx(frameIndex, ti)], 0);
  }

  {
    const uint32_t base = frameIndex * m_threadCount;
    std::memset(m_begun + base, 0, m_threadCount * sizeof(uint8_t));
    std::memset(m_hadWork + base, 0, m_threadCount * sizeof(uint8_t));
  }

  m_heads[frameIndex].store(0, std::memory_order_release);

  return true;
}

bool VkUploadContext::beginFrame(uint32_t frameIndex) {
  if (m_mode != Mode::FrameRing) {
    LOGE("beginFrame called on non-FrameRing context");
    return false;
  }
  return waitAndReset(frameIndex);
}

bool VkUploadContext::beginBatch() {
  if (m_mode != Mode::OneShot) {
    LOGE("beginBatch called on non-OneShot context");
    return false;
  }
  return waitAndReset(0);
}

VkUploadContext::Recorder VkUploadContext::recorder(uint32_t frameIndex,
                                                    uint32_t threadIndex) {
  if (m_ctx == nullptr || m_stagingMapped == nullptr) {
    return {};
  }

  if (threadIndex >= m_threadCount) {
    LOGE("threadIndex is larger than threadCount");
    return {};
  }

  if (m_mode == Mode::OneShot) {
    frameIndex = 0;
  }

  if (frameIndex >= m_framesInFlight) {
    LOGE("frameIndex is larger than framesInFlight");
    return {};
  }

  return Recorder{this, frameIndex, threadIndex};
}

bool VkUploadContext::beginCmd(uint32_t frameIndex, uint32_t threadIndex) {
  VkCommandBuffer cmd = cmdAt(frameIndex, threadIndex);
  if (cmd == VK_NULL_HANDLE) {
    return false;
  }

  VkCommandBufferBeginInfo bufBeginInfo{};
  bufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VkResult res = vkBeginCommandBuffer(cmd, &bufBeginInfo);
  if (res != VK_SUCCESS) {
    LOGE("vkBeginCommandBuffer failed: {}", fmt::underlying(res));
    return false;
  }

  m_begun[idx(frameIndex, threadIndex)] = 1;
  return true;
}

bool VkUploadContext::endCmd(uint32_t frameIndex, uint32_t threadIndex) {
  VkCommandBuffer cmd = cmdAt(frameIndex, threadIndex);
  if (cmd == VK_NULL_HANDLE) {
    return false;
  }

  VkResult res = vkEndCommandBuffer(cmd);
  if (res != VK_SUCCESS) {
    LOGE("vkEndCommandBuffer failed: {}", fmt::underlying(res));
    return false;
  }

  return true;
}

VkStagingAlloc VkUploadContext::allocStaging(uint32_t frameIndex,
                                             VkDeviceSize size,
                                             VkDeviceSize alignment) {
  VkStagingAlloc out{};
  if (m_stagingMapped == nullptr) {
    return out;
  }

  VkDeviceSize a = std::max(alignment, m_bufCopyAlign);
  std::atomic<VkDeviceSize> &head = m_heads[frameIndex];

  // CAS loop to align without wasting slice space
  for (;;) {
    VkDeviceSize cur = head.load(std::memory_order_acquire);
    VkDeviceSize aligned = alignUp(cur, a);
    VkDeviceSize next = aligned + size;

    if (next > m_bytesPerSlice) {
      return out;
    }

    if (head.compare_exchange_weak(cur, next, std::memory_order_acq_rel,
                                   std::memory_order_acquire)) {
      VkDeviceSize absOffset = sliceBase(frameIndex) + aligned;
      out.ptr = static_cast<std::uint8_t *>(m_stagingMapped) + absOffset;
      out.offset = absOffset;
      out.size = size;

      PROFILE_UPLOAD_ADD(UploadProfiler::Stat::StagingUsedBytes, size);

      return out;
    }
  }
}

bool VkUploadContext::submit(uint32_t frameIndex, bool wait) {
  if (m_ctx == nullptr) {
    return false;
  }

  m_submitScratch.clear();

  for (uint32_t ti = 0; ti < m_threadCount; ++ti) {
    const uint32_t k = idx(frameIndex, ti);
    if (m_begun[k] == 0) {
      continue;
    }

    if (m_hadWork[k] == 0) {
      if (!endCmd(frameIndex, ti)) {
        return false;
      }
      continue;
    }

    if (!endCmd(frameIndex, ti)) {
      return false;
    }
    m_submitScratch.push_back(cmdAt(frameIndex, ti));
  }

  if (m_submitScratch.empty()) {
    return true;
  }

  VkFence fence = m_fences[frameIndex];
  VkResult res = vkResetFences(m_ctx->device(), 1, &fence);
  if (res != VK_SUCCESS) {
    LOGE("vkResetFences failed: {}", fmt::underlying(res));
    return false;
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = static_cast<uint32_t>(m_submitScratch.size());
  submitInfo.pCommandBuffers = m_submitScratch.data();

  res = vkQueueSubmit(m_ctx->graphicsQueue(), 1, &submitInfo, fence);
  if (res != VK_SUCCESS) {
    LOGE("vkQueueSubmit failed: {}", fmt::underlying(res));
    return false;
  }

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::UploadSubmitCount);

  if (wait) {
    res = vkWaitForFences(m_ctx->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
      LOGE("vkWaitForFences(wait) failed: {}", fmt::underlying(res));
      return false;
    }
  }

  return true;
}

bool VkUploadContext::flushFrame(uint32_t frameIndex, bool wait) {
  if (m_mode != Mode::FrameRing) {
    LOGE("flushFrame called on non-FrameRing context");
    return false;
  }

  if (frameIndex >= m_framesInFlight) {
    return false;
  }

  return submit(frameIndex, wait);
}

bool VkUploadContext::flushBatch(bool wait) {
  if (m_mode != Mode::OneShot) {
    LOGE("flushBatch called on non-oneShot context");
    return false;
  }

  return submit(0, wait);
}

void VkUploadContext::transitionImage(VkCommandBuffer cmd, VkImage image,
                                      VkImageLayout oldLayout,
                                      VkImageLayout newLayout,
                                      VkPipelineStageFlags finalStage) {
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
    dstStage = finalStage;
    srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstAccess = VK_ACCESS_SHADER_READ_BIT;
  } else {
    LOGE("Unsupported layout transition {} -> {}", fmt::underlying(oldLayout),
         fmt::underlying(newLayout));
    return;
  }

  util::cmdImageBarrier(cmd, image, oldLayout, newLayout, srcAccess, dstAccess,
                        srcStage, dstStage, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkCommandBuffer VkUploadContext::Recorder::cmd() const noexcept {
  if (m_ctx == nullptr) {
    return VK_NULL_HANDLE;
  }

  return m_ctx->cmdAt(m_frameIndex, m_threadIndex);
}

void VkUploadContext::Recorder::ensureBegun() {
  const uint32_t k = m_ctx->idx(m_frameIndex, m_threadIndex);
  if (m_ctx->m_begun[k] != 0) {
    return;
  }

  (void)m_ctx->beginCmd(m_frameIndex, m_threadIndex);
}

VkStagingAlloc VkUploadContext::Recorder::allocStaging(VkDeviceSize size,
                                                       VkDeviceSize alignment) {
  if (m_ctx == nullptr) {
    return {};
  }

  ensureBegun();

  VkStagingAlloc a = m_ctx->allocStaging(m_frameIndex, size, alignment);

  if (a) {
    const uint32_t k = m_ctx->idx(m_frameIndex, m_threadIndex);
    m_ctx->m_hadWork[k] = 1;
  }

  return a;
}

void VkUploadContext::Recorder::cmdCopyToBuffer(VkBuffer dst,
                                                VkDeviceSize dstOffset,
                                                VkDeviceSize srcOffset,
                                                VkDeviceSize size) {
  ensureBegun();

  VkCommandBuffer c = cmd();
  if (c == VK_NULL_HANDLE) {
    return;
  }

  const uint32_t k = m_ctx->idx(m_frameIndex, m_threadIndex);
  m_ctx->m_hadWork[k] = 1;

  VkBufferCopy copy{};
  copy.srcOffset = srcOffset;
  copy.dstOffset = dstOffset;
  copy.size = size;
  vkCmdCopyBuffer(c, m_ctx->m_staging.handle(), dst, 1, &copy);
}

void VkUploadContext::Recorder::cmdBarrierBufferTransferToShader(
    VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size,
    VkPipelineStageFlags dstStage) {
  ensureBegun();

  VkCommandBuffer c = cmd();
  if (c == VK_NULL_HANDLE) {
    return;
  }

  const uint32_t k = m_ctx->idx(m_frameIndex, m_threadIndex);
  m_ctx->m_hadWork[k] = 1;

  util::cmdBufferBarrier(c, buffer, offset, size, VK_ACCESS_TRANSFER_WRITE_BIT,
                         VK_ACCESS_SHADER_READ_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage);
}

void VkUploadContext::Recorder::cmdUploadRGBA8ToImage(
    VkImage image, uint32_t width, uint32_t height, VkDeviceSize srcOffset,
    VkImageLayout finalLayout, VkPipelineStageFlags finalStage) {
  ensureBegun();

  VkCommandBuffer c = cmd();
  if (c == VK_NULL_HANDLE) {
    return;
  }

  const uint32_t k = m_ctx->idx(m_frameIndex, m_threadIndex);
  m_ctx->m_hadWork[k] = 1;

  transitionImage(c, image, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalStage);

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

  vkCmdCopyBufferToImage(c, m_ctx->m_staging.handle(), image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  transitionImage(c, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout,
                  finalStage);
}
