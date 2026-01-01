#include "backend/profiling/profiling_logger.hpp"

#include "backend/profiling/cpu_profiler.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/profiling/vk_gpu_profiler.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>

namespace profiling {

static inline void formatBytes(char *out, std::size_t outSize,
                               std::uint64_t b) noexcept {
  const char *suffix = "B";
  double v = static_cast<double>(b);

  if (b >= (1ULL << 30)) {
    v /= static_cast<double>(1ULL << 30);
    suffix = "GiB";
  } else if (b >= (1ULL << 20)) {
    v /= static_cast<double>(1ULL << 20);
    suffix = "MiB";
  } else if (b >= (1ULL << 10)) {
    v /= static_cast<double>(1ULL << 10);
    suffix = "KiB";
  }

  const int n = std::snprintf(out, outSize, "%.2f%s", v, suffix);
  if (n < 0) {
    // encoding/format error
    out[0] = '\0';
    return;
  }

  // If truncated, snprintf prints out null-terminate, but to be explicit
  out[outSize - 1] = '\0';
}

bool FrameLogger::shouldLog() noexcept {
  if (m_period == 0) {
    return false;
  }

  ++m_frameCounter;
  return (m_frameCounter % m_period) == 0ULL;
}

static void logCpu(const CpuProfiler &cpu) noexcept {
  const auto &st = cpu.last();

  std::cerr << "\n[Profiler]\n CPU ms: frame="
            << st.ms[(size_t)CpuProfiler::Stat::FrameTotal]
            << " acquire=" << st.ms[(size_t)CpuProfiler::Stat::Acquire]
            << " waitForFence="
            << st.ms[(size_t)CpuProfiler::Stat::WaitForFence]
            << " ubo=" << st.ms[(size_t)CpuProfiler::Stat::UpdatePerFrameUBO]
            << " record=" << st.ms[(size_t)CpuProfiler::Stat::RecordCmd]

            << " queueSubmit=" << st.ms[(size_t)CpuProfiler::Stat::QueueSubmit]
            << " queuePresent="
            << st.ms[(size_t)CpuProfiler::Stat::QueuePresent]
            << " other=" << st.ms[(size_t)CpuProfiler::Stat::Other] << "\n"
            << " draws=" << st.drawCalls << " triangles=" << st.triangles
            << " pipeBinds=" << st.pipelineBinds
            << " descBinds=" << st.descriptorBinds << "\n";
}

static void logGpu(const VkGpuProfiler &gpu) noexcept {
  const auto &gst = gpu.last();
  if (gst.valid) {
    std::cerr << " gpuFrame=" << gst.frameMs
              << " gpuMainPass=" << gst.mainPassMs
              << " gpuIdleGap=" << gst.idleGapMs << "\n";
  }
}

static void logUpload(const UploadProfiler &upload) noexcept {
  const auto &ust = upload.last();
  const auto idx = [](UploadProfiler::Stat stat) {
    return static_cast<std::size_t>(stat);
  };

  const std::uint64_t submitCount =
      ust.v[idx(UploadProfiler::Stat::UploadSubmitCount)];

  const std::uint64_t memcpyCount =
      ust.v[idx(UploadProfiler::Stat::UploadMemcpyCount)];
  const std::uint64_t memcpyBytes =
      ust.v[idx(UploadProfiler::Stat::UploadMemcpyBytes)];

  const std::uint64_t stagingCreatedCount =
      ust.v[idx(UploadProfiler::Stat::StagingCreatedCount)];
  const std::uint64_t stagingAllocBytes =
      ust.v[idx(UploadProfiler::Stat::StagingAllocatedBytes)];
  const std::uint64_t stagingUsedBytes =
      ust.v[idx(UploadProfiler::Stat::StagingUsedBytes)];

  const std::uint64_t bufCount =
      ust.v[idx(UploadProfiler::Stat::BufferUploadCount)];
  const std::uint64_t bufBytes =
      ust.v[idx(UploadProfiler::Stat::BufferUploadBytes)];
  const std::uint64_t bufAllocBytes =
      ust.v[idx(UploadProfiler::Stat::BufferAllocatedBytes)];

  const std::uint64_t texCount =
      ust.v[idx(UploadProfiler::Stat::TextureUploadCount)];
  const std::uint64_t texBytes =
      ust.v[idx(UploadProfiler::Stat::TextureUploadBytes)];
  const std::uint64_t texAllocBytes =
      ust.v[idx(UploadProfiler::Stat::TextureAllocatedBytes)];

  std::array<char, 32> memcpyStr{};
  std::array<char, 32> stagingAllocStr{};
  std::array<char, 32> stagingUsedStr{};
  std::array<char, 32> bufStr{};
  std::array<char, 32> bufAllocStr{};
  std::array<char, 32> texStr{};
  std::array<char, 32> texAllocStr{};

  formatBytes(memcpyStr.data(), sizeof(memcpyStr), memcpyBytes);
  formatBytes(stagingAllocStr.data(), sizeof(stagingAllocStr),
              stagingAllocBytes);
  formatBytes(stagingUsedStr.data(), sizeof(stagingUsedStr), stagingUsedBytes);
  formatBytes(bufStr.data(), sizeof(bufStr), bufBytes);
  formatBytes(bufAllocStr.data(), sizeof(bufAllocStr), bufAllocBytes);
  formatBytes(texStr.data(), sizeof(texStr), texBytes);
  formatBytes(texAllocStr.data(), sizeof(texAllocStr), texAllocBytes);

  // If prints show 0 here, its likely the scene if only static meshes
  std::cerr << " submits=" << submitCount
            << " staging=(created=" << stagingCreatedCount
            << " alloc=" << stagingAllocStr.data()
            << " used=" << stagingUsedStr.data() << ")"
            << " memcpy=(count=" << memcpyCount << " size=" << memcpyStr.data()
            << ")"
            << " buf=(uploads=" << bufCount << " bytes=" << bufStr.data()
            << " alloc=" << bufAllocStr.data() << ")"
            << " tex=(uploads=" << texCount << " bytes=" << texStr.data()
            << " alloc=" << texAllocStr.data() << ")"
            << "\n";
}

void FrameLogger::logPerFrame(const CpuProfiler &cpu, const VkGpuProfiler &gpu,
                              const UploadProfiler &upload) noexcept {

  if (!shouldLog()) {
    return;
  }

  // NOTE: if queueSubmit is large its likely artifical wait time
  // for vsync from FIFO present mode in swapchain

  // Note: Most of other is likely from the vkWaitForFence in commands on
  // submit immediate. I didn't bother logging this since its a pain to
  // pass since mesh_store is the one who calls the uploaders who then
  // call submit immediate so I would have to pass profiler a lot. But it
  // doesn't matter since eventually submit Immediate will be removed and
  // we won't have blocking anymore
  logCpu(cpu);

  logUpload(upload);
  logGpu(gpu);
}

#ifndef NDEBUG
void emit(Event e, double ms) noexcept {
  std::cerr << "[Event] " << name(e) << " ms=" << ms << "\n";
}
#endif

} // namespace profiling
