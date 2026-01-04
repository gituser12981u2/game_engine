#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"

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
            VkDeviceSize staticBudgetPerFrame, VkDeviceSize frameBudgetPerFrame,
            UploadProfiler *profiler);
  void shutdown() noexcept;

  // TODO: make static not per frame after uploader is
  // redone to allow for per thread command pools
  bool beginFrame(uint32_t frameIndex);

  bool flushFrame(bool wait);
  bool flushStatic(bool wait);

  bool flushAll(bool wait);

  [[nodiscard]] VkUploadContext &statik() noexcept { return m_static; }
  [[nodiscard]] VkUploadContext &frame() noexcept { return m_frame; }

  [[nodiscard]] uint32_t framesInFlight() const noexcept {
    return m_framesInFlight;
  }

private:
  VkBackendCtx *m_ctx = nullptr; // non-owning
  uint32_t m_framesInFlight = 0;

  VkUploadContext m_static;
  VkUploadContext m_frame;
};
