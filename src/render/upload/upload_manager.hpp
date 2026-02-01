#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkBackendCtx;

class UploadManager {
public:
  UploadManager() = default;
  ~UploadManager() noexcept { shutdown(); }

  UploadManager(const UploadManager &) = delete;
  UploadManager &operator=(const UploadManager &) = delete;

  UploadManager(UploadManager &&other) noexcept { *this = std::move(other); }
  UploadManager &operator=(UploadManager &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_ctx = other.m_ctx;
    other.m_ctx = nullptr;
    m_framesInFlight = other.m_framesInFlight;
    other.m_framesInFlight = 0;
    m_static = std::move(other.m_static);
    m_frame = std::move(other.m_frame);

    return *this;
  }

  bool init(VkBackendCtx &ctx, uint32_t framesInFlight,
            VkDeviceSize staticTotalBytes, VkDeviceSize frameBudget,
            uint32_t threadCount);
  void shutdown() noexcept;

  bool beginFrame(uint32_t frameIndex);
  bool flushFrame(bool wait);
  [[nodiscard]] VkUploadContext::Recorder frameRecorder(uint32_t threadIndex);

  bool beginStatic();
  bool flushStatic(bool wait);
  [[nodiscard]] VkUploadContext::Recorder staticRecorder(uint32_t threadIndex);

  bool flushAll(bool wait);

  [[nodiscard]] uint32_t currentFrameIndex() const noexcept {
    return m_currentFrameIndex;
  }
  [[nodiscard]] uint32_t framesInFlight() const noexcept {
    return m_framesInFlight;
  }
  [[nodiscard]] uint32_t threadCount() const noexcept { return m_threadCount; }

private:
  VkBackendCtx *m_ctx = nullptr; // non-owning

  uint32_t m_framesInFlight = 0;
  uint32_t m_threadCount = 0;

  uint32_t m_currentFrameIndex = 0;
  bool m_frameBegun = false;

  bool m_staticActive = false;

  VkUploadContext m_static;
  VkUploadContext m_frame;
};
