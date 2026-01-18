#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/buffers/vk_buffer.hpp"

#include <atomic>
#include <cstdint>
#include <utility>
#include <vector>
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
  enum class Mode : uint8_t {
    FrameRing, // slice per frameIndex in [0, framesInflight)
    OneShot,   // single reusable batch (not per frame)
  };

  class Recorder {
  public:
    Recorder() = default;

    // Allocate space in the staging slice for the current frame.
    VkStagingAlloc allocStaging(VkDeviceSize size, VkDeviceSize alignment = 16);

    // Record a copy from staging -> buffer.
    void cmdCopyToBuffer(VkBuffer dst, VkDeviceSize dstOffset,
                         VkDeviceSize srcOffset, VkDeviceSize size);

    // Record a buffer -> image upload for RGBA8 with layout transition:
    // UNDEFINED -> TRANSFER_DST_OPTIMAL -> finalLayout
    void cmdUploadRGBA8ToImage(VkImage image, uint32_t width, uint32_t height,
                               VkDeviceSize srcOffset,
                               VkImageLayout finalLayout,
                               VkPipelineStageFlags finalStage);

    void cmdBarrierBufferTransferToShader(VkBuffer buffer, VkDeviceSize offset,
                                          VkDeviceSize size,
                                          VkPipelineStageFlags dstStage);

    [[nodiscard]] VkCommandBuffer cmd() const noexcept;

    explicit operator bool() const noexcept { return m_ctx != nullptr; }

  private:
    friend class VkUploadContext;
    Recorder(VkUploadContext *ctx, uint32_t frameIndex, uint32_t threadIndex)
        : m_ctx(ctx), m_frameIndex(frameIndex), m_threadIndex(threadIndex) {}

    void ensureBegun();

    VkUploadContext *m_ctx = nullptr; // non-owning
    uint32_t m_frameIndex = 0;
    uint32_t m_threadIndex = 0;
  };

  VkUploadContext() = default;
  ~VkUploadContext() noexcept { shutdown(); }

  VkUploadContext(const VkUploadContext &) = delete;
  VkUploadContext &operator=(const VkUploadContext &) = delete;

  VkUploadContext(VkUploadContext &&other) noexcept {
    *this = std::move(other);
  }
  VkUploadContext &operator=(VkUploadContext &&other) noexcept;

  // perFrameBytes: bytes reserved for each frame slice
  bool initFrameRing(VkBackendCtx &ctx, uint32_t framesInFlight,
                     VkDeviceSize bytesPerFrameSlice, uint32_t threadCount);

  bool initOneShot(VkBackendCtx &ctx, VkDeviceSize totalBytes,
                   uint32_t threadCount);

  void shutdown() noexcept;

  [[nodiscard]] Mode mode() const noexcept { return m_mode; }
  [[nodiscard]] uint32_t framesInFlight() const noexcept {
    return m_framesInFlight;
  }
  [[nodiscard]] uint32_t threadCount() const noexcept { return m_threadCount; }

  [[nodiscard]] VkBuffer stagingBuffer() const noexcept {
    return m_staging.handle();
  }
  [[nodiscard]] VkDeviceSize bytesPerSlice() const noexcept {
    return m_bytesPerSlice;
  }

  // This waits for the fence associated with this frame slice,
  // resets the cmd pool, and beings recording.
  bool beginFrame(uint32_t frameIndex);
  bool beginBatch();

  // Get a recorder for this frame/batch and thread index
  // Note: for OneShot mode, frameIndex is ignored (use 0)
  [[nodiscard]] Recorder recorder(uint32_t frameIndex, uint32_t threadIndex);

  // If wait=true, wait for completion.
  bool flushFrame(uint32_t frameIndex, bool wait);
  bool flushBatch(bool wait);

private:
  bool initCommon(VkBackendCtx &ctx, Mode mode, uint32_t framesInflight,
                  VkDeviceSize bytesPerFrameSlice, uint32_t threadCount);

  static VkDeviceSize alignUp(VkDeviceSize v, VkDeviceSize a) noexcept;

  bool waitAndReset(uint32_t frameIndex);
  bool submit(uint32_t frameIndex, bool wait);

  bool beginCmd(uint32_t frameIndex, uint32_t threadIndex);
  bool endCmd(uint32_t frameIndex, uint32_t threadIndex);

  VkStagingAlloc allocStaging(uint32_t frameIndex, VkDeviceSize size,
                              VkDeviceSize alignment);

  void transitionImage(VkCommandBuffer cmd, VkImage image,
                       VkImageLayout oldLayout, VkImageLayout newLayout,
                       VkPipelineStageFlags finalStage);

  [[nodiscard]] uint32_t idx(uint32_t frameIndex,
                             uint32_t threadIndex) const noexcept {
    return (frameIndex * m_threadCount) + threadIndex;
  }
  [[nodiscard]] VkDeviceSize sliceBase(uint32_t frameIndex) const noexcept {
    return VkDeviceSize(frameIndex) * m_bytesPerSlice;
  }

  [[nodiscard]] VkCommandPool poolAt(uint32_t frameIndex,
                                     uint32_t threadIndex) const noexcept;
  [[nodiscard]] VkCommandBuffer cmdAt(uint32_t frameIndex,
                                      uint32_t threadIndex) const noexcept;

  VkBackendCtx *m_ctx = nullptr; // non-owning

  Mode m_mode = Mode::FrameRing;

  uint32_t m_framesInFlight = 0;
  uint32_t m_threadCount = 0;
  VkDeviceSize m_bytesPerSlice = 0;

  VkDeviceSize m_bufCopyAlign = 1;
  VkDeviceSize m_rowPitchAlign = 1;

  VkBufferObj m_staging;
  void *m_stagingMapped = nullptr;

  VkCommandPool *m_pools = nullptr;
  VkCommandBuffer *m_cmds = nullptr;
  VkFence *m_fences = nullptr;

  // Per-frame atomic head into slice (offset within slice)
  std::atomic<VkDeviceSize> *m_heads = nullptr;

  uint8_t *m_begun = nullptr;
  uint8_t *m_hadWork = nullptr;

  std::vector<VkCommandBuffer> m_submitScratch;
};
