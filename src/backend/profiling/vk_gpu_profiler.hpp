#pragma once

#include "backend/core/vk_backend_ctx.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkBackendCtx;

class VkGpuProfiler {
public:
  enum class Marker : std::uint8_t {
    FrameBegin = 0,
    MainPassBegin,
    MainPassEnd,
    FrameEnd,
    Count
  };

  struct GpuFrameStats {
    bool valid = false;
    double frameMs = 0.0;
    double mainPassMs = 0.0;
    double idleGapMs = 0.0;
  };

  VkGpuProfiler() = default;
  ~VkGpuProfiler() noexcept { shutdown(); }

  VkGpuProfiler(const VkGpuProfiler &) = delete;
  VkGpuProfiler &operator=(const VkGpuProfiler &) = delete;

  VkGpuProfiler(VkGpuProfiler &&other) noexcept { *this = std::move(other); }
  VkGpuProfiler &operator=(VkGpuProfiler &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
    m_framesInFlight = std::exchange(other.m_framesInFlight, 0U);
    m_timestampPeriodNs = std::exchange(other.m_timestampPeriodNs, 0.0);
    m_last = other.m_last;
    m_submittedFrames = std::exchange(other.m_submittedFrames, 0ULL);

    return *this;
  }

  bool init(const VkBackendCtx &ctx, uint32_t framesInFlight);
  void shutdown() noexcept;

  void beginFrameCmd(VkCommandBuffer cmd, uint32_t frameIndex) noexcept;

  void markFrameBegin(VkCommandBuffer cmd, uint32_t frameIndex) noexcept;
  void markMainPassBegin(VkCommandBuffer cmd, uint32_t frameIndex) noexcept;
  void markMainPassEnd(VkCommandBuffer cmd, uint32_t frameIndex) noexcept;
  void markFrameEnd(VkCommandBuffer cmd, uint32_t frameIndex) noexcept;

  void onFrameSubmitted() noexcept { ++m_submittedFrames; }

  [[nodiscard]] GpuFrameStats tryCollect(uint32_t frameIndex) noexcept;
  [[nodiscard]] const GpuFrameStats &last() const noexcept { return m_last; }

private:
  static constexpr uint32_t markersPerFrame() noexcept {
    return static_cast<uint32_t>(VkGpuProfiler::Marker::Count);
  }

  static constexpr uint32_t base(uint32_t frameIndex) noexcept {
    return frameIndex * markersPerFrame();
  }

  void writeTs(VkCommandBuffer cmd, uint32_t frameIndex, Marker marker,
               VkPipelineStageFlagBits stage) noexcept;

  VkDevice m_device = VK_NULL_HANDLE; // non-owning
  VkQueryPool m_pool = VK_NULL_HANDLE;

  uint32_t m_framesInFlight = 0;
  double m_timestampPeriodNs = 0.0; // ns per tick
  std::uint64_t m_submittedFrames = 0;

  std::uint64_t m_lastFrameEndTs = 0;
  bool m_haveLastFrameEndTs = false;

  GpuFrameStats m_last{};
};
