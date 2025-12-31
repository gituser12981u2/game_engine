#include "backend/profiling/vk_gpu_profiler.hpp"

#include "backend/core/vk_backend_ctx.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkGpuProfiler::init(const VkBackendCtx &ctx, uint32_t framesInFlight) {
  if (framesInFlight == 0) {
    std::cerr << "[GpuProfiler] framesInFlight equals 0\n";
    return false;
  }

  shutdown();

  VkPhysicalDevice physicalDevice = ctx.physicalDevice();
  VkDevice device = ctx.device();

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(physicalDevice, &props);

  if (props.limits.timestampPeriod <= 0.0F) {
    std::cerr << "[GpuProfiler] timestampPeriod is invalid\n";
    return false;
  }

  // If timestampComputeAndGraphics is false, grahics timestamps may be invalid.
  if (props.limits.timestampComputeAndGraphics == VK_FALSE) {
    std::cerr << "[GpuProfiler] timestampComputeAndGraphics is false\n";
    return false;
  }

  m_device = device;
  m_framesInFlight = framesInFlight;
  m_timestampPeriodNs = static_cast<double>(props.limits.timestampPeriod);
  m_lastFrameEndTs = 0;
  m_haveLastFrameEndTs = false;

  const uint32_t totalQueries = m_framesInFlight * markersPerFrame();

  VkQueryPoolCreateInfo queryPoolInfo{};
  queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
  queryPoolInfo.queryCount = totalQueries;

  if (vkCreateQueryPool(m_device, &queryPoolInfo, nullptr, &m_pool) !=
      VK_SUCCESS) {
    std::cerr << "[GpuProfiler] Failed to create VkQueryPool\n";
    shutdown();
    return false;
  }

  m_submittedFrames = 0;
  m_last = {};
  return true;
}

void VkGpuProfiler::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
    vkDestroyQueryPool(m_device, m_pool, nullptr);
  }

  m_device = VK_NULL_HANDLE;
  m_pool = VK_NULL_HANDLE;
  m_framesInFlight = 0;
  m_timestampPeriodNs = 0.0;
  m_submittedFrames = 0;
  m_lastFrameEndTs = 0;
  m_haveLastFrameEndTs = false;
  m_last = {};
}

void VkGpuProfiler::writeTs(VkCommandBuffer cmd, uint32_t frameIndex,
                            Marker marker,
                            VkPipelineStageFlagBits stage) noexcept {
  if (m_pool == VK_NULL_HANDLE) {
    return;
  }

  const uint32_t query = base(frameIndex) + static_cast<uint32_t>(marker);
  vkCmdWriteTimestamp(cmd, stage, m_pool, query);
}

void VkGpuProfiler::beginFrameCmd(VkCommandBuffer cmd,
                                  uint32_t frameIndex) noexcept {
  if (m_pool == VK_NULL_HANDLE) {
    return;
  }

  const uint32_t b = base(frameIndex);
  // Requires VK 1.2
  vkCmdResetQueryPool(cmd, m_pool, b, markersPerFrame());
}

void VkGpuProfiler::markFrameBegin(VkCommandBuffer cmd,
                                   uint32_t frameIndex) noexcept {
  writeTs(cmd, frameIndex, Marker::FrameBegin,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}

void VkGpuProfiler::markMainPassBegin(VkCommandBuffer cmd,
                                      uint32_t frameIndex) noexcept {
  writeTs(cmd, frameIndex, Marker::MainPassBegin,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}

void VkGpuProfiler::markMainPassEnd(VkCommandBuffer cmd,
                                    uint32_t frameIndex) noexcept {

  writeTs(cmd, frameIndex, Marker::MainPassEnd,
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

void VkGpuProfiler::markFrameEnd(VkCommandBuffer cmd,
                                 uint32_t frameIndex) noexcept {
  writeTs(cmd, frameIndex, Marker::FrameEnd,
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

VkGpuProfiler::GpuFrameStats
VkGpuProfiler::tryCollect(uint32_t frameIndex) noexcept {
  GpuFrameStats out{};

  if (m_pool == VK_NULL_HANDLE || m_framesInFlight == 0) {
    return out;
  }

  if (m_submittedFrames < m_framesInFlight) {
    return out;
  }

  // Read the previous frame to avoid stalling
  const uint32_t prev = (frameIndex + m_framesInFlight - 1U) % m_framesInFlight;

  const uint32_t firstQuery = base(prev);
  const uint32_t qCountU32 = markersPerFrame();

  static constexpr std::size_t kQueryCount =
      static_cast<std::size_t>(Marker::Count);
  static constexpr std::size_t kU64sPerQuery = 2;

  // Request value and availability for each query in layout:
  // [value0, avail0, value1, avail1, ...]
  // std::array<std::uint64_t, queryCount * 2> data{};
  std::array<std::uint64_t, kQueryCount * kU64sPerQuery> data{};

  VkResult res = vkGetQueryPoolResults(
      m_device, m_pool, firstQuery, qCountU32, sizeof(data), data.data(),
      sizeof(std::uint64_t) * kU64sPerQuery,
      VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

  if (res == VK_NOT_READY) {
    return out;
  }
  if (res != VK_SUCCESS) {
    std::cerr << "[GpuProfiler] vkGetQueryPoolResults failed: " << res << "\n";
    return out;
  }

  // Require all markers be available
  for (std::size_t i = 0; i < kQueryCount; ++i) {
    if (data[(i * kU64sPerQuery) + 1] == 0) {
      return out;
    }
  }

  auto readVal = [&](Marker m) -> std::uint64_t {
    const uint32_t i = static_cast<uint32_t>(m);
    return data[(i * kU64sPerQuery) + 0];
  };

  const std::uint64_t t0 = readVal(Marker::FrameBegin);
  const std::uint64_t t1 = readVal(Marker::FrameEnd);
  const std::uint64_t mp0 = readVal(Marker::MainPassBegin);
  const std::uint64_t mp1 = readVal(Marker::MainPassEnd);

  if (t1 <= t0 || mp1 <= mp0) {
    return out;
  }

  const double frameNs = static_cast<double>(t1 - t0) * m_timestampPeriodNs;
  const double mainNs = static_cast<double>(mp1 - mp0) * m_timestampPeriodNs;

  out.valid = true;
  out.frameMs = frameNs / 1e6;
  out.mainPassMs = mainNs / 1e6;

  out.idleGapMs = 0.0;
  if (m_haveLastFrameEndTs) {
    if (t0 > m_lastFrameEndTs) {
      const double gapNs =
          static_cast<double>(t0 - m_lastFrameEndTs) * m_timestampPeriodNs;
      out.idleGapMs = gapNs / 1e6;
    }
  }

  m_lastFrameEndTs = t1;
  m_haveLastFrameEndTs = true;

  m_last = out;
  return out;
}
