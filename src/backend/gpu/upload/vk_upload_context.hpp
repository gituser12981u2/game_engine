#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/buffers/vk_buffer.hpp"
#include "backend/profiling/upload_profiler.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

struct VkStagingAlloc {
  void *ptr = nullptr;     // mapped CPU pointer to write info
  VkDeviceSize offset = 0; // absolute offset into staging buffer
  VkDeviceSize size = 0;   // requested size
  explicit operator bool() const noexcept { return ptr != nullptr; }
};

// TODO: check for transfer queue in queue family and use it
// TODO: use timeline semaphore values to know when upload is complete
// instead of offloading submits to a different command buffer

class VkUploadContext {
public:
  VkUploadContext() = default;
  ~VkUploadContext() noexcept { shutdown(); }

  VkUploadContext(const VkUploadContext &) = delete;
  VkUploadContext &operator=(const VkUploadContext &) = delete;

  VkUploadContext(VkUploadContext &&other) noexcept {
    *this = std::move(other);
  }
  VkUploadContext &operator=(VkUploadContext &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_ctx = std::exchange(other.m_ctx, nullptr);
    m_profiler = std::exchange(other.m_profiler, nullptr);

    m_framesInFlight = std::exchange(other.m_framesInFlight, 0);
    m_frameIndex = std::exchange(other.m_frameIndex, 0);
    m_perFrameBytes = std::exchange(other.m_perFrameBytes, 0);

    m_bufCopyAlign = std::exchange(other.m_bufCopyAlign, 1);
    m_rowPitchAlign = std::exchange(other.m_rowPitchAlign, 1);

    m_staging = std::move(other.m_staging);
    m_stagingMapped = std::exchange(other.m_stagingMapped, nullptr);

    m_pools = std::exchange(other.m_pools, nullptr);
    m_cmds = std::exchange(other.m_cmds, nullptr);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
    m_cmd = std::exchange(other.m_cmd, VK_NULL_HANDLE);

    m_fences = std::exchange(other.m_fences, nullptr);

    m_sliceBase = std::exchange(other.m_sliceBase, 0);
    m_sliceHead = std::exchange(other.m_sliceHead, 0);
    m_recording = std::exchange(other.m_recording, false);

    return *this;
  }

  // perFrameBytes: bytes reserved for each frame slice
  bool init(VkBackendCtx &ctx, uint32_t framesInflight,
            VkDeviceSize perFrameBytes, UploadProfiler *profiler);
  void shutdown() noexcept;

  // This waits for the fence associated with this frame slice,
  // resets the cmd pool, and beings recording.
  bool beginFrame(uint32_t frameIndex);

  // Allocate space in the staging slice for the current frame.
  VkStagingAlloc allocStaging(VkDeviceSize size, VkDeviceSize alignment = 16);

  // Record a copy from staging -> buffer.
  void cmdCopyToBuffer(VkBuffer dst, VkDeviceSize dstOffset,
                       VkDeviceSize srcOffset, VkDeviceSize size);

  // Record a buffer -> image upload for RGBA8 with layout transition:
  // UNDEFINED -> TRANSFER_DST_OPTIMAL -> finalLayout
  void cmdUploadRGBA8ToImage(VkImage image, uint32_t width, uint32_t height,
                             VkDeviceSize srcOffset, VkImageLayout finalLayout);

  void cmdBarrierBufferTransferToVertexShader(VkBuffer buffer,
                                              VkDeviceSize offset,
                                              VkDeviceSize size);

  void cmdBarrierBufferTransferToFragmentShader(VkBuffer buffer,
                                                VkDeviceSize offset,
                                                VkDeviceSize size);

  // If wait=true, wait for completion.
  bool flush(bool wait);

  [[nodiscard]] VkCommandBuffer cmd() const noexcept {
    return (m_cmds != nullptr && m_frameIndex < m_framesInFlight)
               ? m_cmds[m_frameIndex]
               : VK_NULL_HANDLE;
  }
  [[nodiscard]] VkBuffer stagingBuffer() const noexcept {
    return m_staging.handle();
  }
  [[nodiscard]] VkDeviceSize perFrameBytes() const noexcept {
    return m_perFrameBytes;
  }
  [[nodiscard]] uint32_t framesInflight() const noexcept {
    return m_framesInFlight;
  }

private:
  static VkDeviceSize alignUp(VkDeviceSize v, VkDeviceSize a) noexcept;

  void transitionImage(VkImage image, VkImageLayout oldLayout,
                       VkImageLayout newLayout);

  bool beginCmd();
  bool endCmd();

  VkBackendCtx *m_ctx = nullptr;        // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning

  uint32_t m_framesInFlight = 0;
  uint32_t m_frameIndex = 0;
  VkDeviceSize m_perFrameBytes = 0;

  VkDeviceSize m_bufCopyAlign = 1;
  VkDeviceSize m_rowPitchAlign = 1;

  VkBufferObj m_staging;
  void *m_stagingMapped = nullptr;

  // TODO: make command pool and buffer per thread instead of per frame
  VkCommandPool *m_pools = VK_NULL_HANDLE;
  VkCommandBuffer *m_cmds = VK_NULL_HANDLE;
  VkCommandPool m_pool = VK_NULL_HANDLE;
  VkCommandBuffer m_cmd = VK_NULL_HANDLE;

  VkFence *m_fences = nullptr;

  VkDeviceSize m_sliceBase = 0;
  VkDeviceSize m_sliceHead = 0;
  bool m_recording = false;
  bool m_hadWork = false;
};
