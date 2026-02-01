#include "backend/profiling/logging/console_sink.hpp"

#if defined(ENABLE_TELEMETRY)
#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/gpu_frame_stats.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include "backend/profiling/telemetry/publish.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"

#include <array>
#include <cstddef>
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

static inline double msAt(const CpuProfiler::Frame &st,
                          CpuProfiler::Stat stat) noexcept {
  return st.ms[static_cast<size_t>(stat)];
}

static inline void formatMs(char *out, size_t outSize, double ms) noexcept {
  if (ms >= 10.0) {
    ignore_snprintf(std::snprintf(out, outSize, "%6.2f", ms));
  } else if (ms >= 1.0) {
    ignore_snprintf(std::snprintf(out, outSize, "%6.3f", ms));
  } else if (ms >= 0.01) {
    ignore_snprintf(std::snprintf(out, outSize, "%6.4f", ms));
  } else if (ms > 0.0) {
    ignore_snprintf(std::snprintf(out, outSize, "%6.6f", ms));
  } else {
    ignore_snprintf(std::snprintf(out, outSize, "%6.6f", 0.0));
  }
}

static void logCpuFrame(const CpuProfiler::Frame &cpu) noexcept {
  std::array<char, 512> line1{};
  std::array<char, 256> line2{};

  std::array<char, 16> frame{};
  std::array<char, 16> acq{};
  std::array<char, 16> fence{};
  std::array<char, 16> ubo{};
  std::array<char, 16> rec{};
  std::array<char, 16> sub{};
  std::array<char, 16> pres{};
  std::array<char, 16> other{};

  formatMs(frame.data(), frame.size(),
           msAt(cpu, CpuProfiler::Stat::FrameTotal));
  formatMs(acq.data(), acq.size(), msAt(cpu, CpuProfiler::Stat::Acquire));
  formatMs(fence.data(), fence.size(),
           msAt(cpu, CpuProfiler::Stat::WaitForFence));
  formatMs(ubo.data(), ubo.size(),
           msAt(cpu, CpuProfiler::Stat::UpdatePerFrameUBO));
  formatMs(rec.data(), rec.size(), msAt(cpu, CpuProfiler::Stat::RecordCmd));
  formatMs(sub.data(), sub.size(), msAt(cpu, CpuProfiler::Stat::QueueSubmit));
  formatMs(pres.data(), pres.size(),
           msAt(cpu, CpuProfiler::Stat::QueuePresent));
  formatMs(other.data(), other.size(), msAt(cpu, CpuProfiler::Stat::Other));

  ignore_snprintf(std::snprintf(
      line1.data(), line1.size(),
      "CPU  ms: frame %s  acq %s  fence %s  ubo %s  rec %s  sub %s  "
      "pres %s  other %s",
      frame.data(), acq.data(), fence.data(), ubo.data(), rec.data(),
      sub.data(), pres.data(), other.data()));

  ignore_snprintf(std::snprintf(
      line2.data(), line2.size(),
      "CPU cnt: draws %-6u inst %-6u tris %-8llu pipe %-4u desc %-4u",
      cpu.drawCalls, cpu.instances,
      static_cast<unsigned long long>(cpu.triangles), cpu.pipelineBinds,
      cpu.descriptorBinds));

  std::cerr << line1.data() << "\n" << line2.data() << "\n";
}

static void logUploadFrame(const UploadProfiler::Frame &upload,
                           const UploadProfiler::Frame &lifetime) noexcept {
  const auto idx = [](UploadProfiler::Stat stat) {
    return static_cast<std::size_t>(stat);
  };

  // per frame
  const std::uint64_t submitCount =
      upload.v[idx(UploadProfiler::Stat::UploadSubmitCount)];

  const std::uint64_t memcpyCount =
      upload.v[idx(UploadProfiler::Stat::UploadMemcpyCount)];
  const std::uint64_t memcpyBytes =
      upload.v[idx(UploadProfiler::Stat::UploadMemcpyBytes)];

  const std::uint64_t stagingUsedBytes =
      upload.v[idx(UploadProfiler::Stat::StagingUsedBytes)];

  const std::uint64_t bufCount =
      upload.v[idx(UploadProfiler::Stat::BufferUploadCount)];
  const std::uint64_t bufBytes =
      upload.v[idx(UploadProfiler::Stat::BufferUploadBytes)];

  const std::uint64_t texCount =
      upload.v[idx(UploadProfiler::Stat::TextureUploadCount)];
  const std::uint64_t texBytes =
      upload.v[idx(UploadProfiler::Stat::TextureUploadBytes)];

  const std::uint64_t matCount =
      upload.v[idx(UploadProfiler::Stat::MaterialUploadCount)];
  const std::uint64_t matBytes =
      upload.v[idx(UploadProfiler::Stat::MaterialUploadBytes)];

  const std::uint64_t instCount =
      upload.v[idx(UploadProfiler::Stat::InstanceUploadCount)];
  const std::uint64_t instBytes =
      upload.v[idx(UploadProfiler::Stat::InstanceUploadBytes)];

  // lifetime
  const std::uint64_t stagingCreatedCount =
      lifetime.v[idx(UploadProfiler::Stat::StagingCreatedCount)];

  const std::uint64_t stagingAllocBytes =
      lifetime.v[idx(UploadProfiler::Stat::StagingAllocatedBytes)];
  const std::uint64_t bufAllocBytes =
      lifetime.v[idx(UploadProfiler::Stat::BufferAllocatedBytes)];
  const std::uint64_t texAllocBytes =
      lifetime.v[idx(UploadProfiler::Stat::TextureAllocatedBytes)];
  const std::uint64_t matAllocBytes =
      lifetime.v[idx(UploadProfiler::Stat::MaterialAllocatedBytes)];
  const std::uint64_t instAllocBytes =
      lifetime.v[idx(UploadProfiler::Stat::InstanceAllocatedBytes)];

  std::array<char, 32> memcpyStr{};
  std::array<char, 32> stagingUsedStr{};
  std::array<char, 32> bufStr{};
  std::array<char, 32> texStr{};
  std::array<char, 32> matStr{};
  std::array<char, 32> instStr{};

  std::array<char, 32> stagingAllocStr{};
  std::array<char, 32> bufAllocStr{};
  std::array<char, 32> texAllocStr{};
  std::array<char, 32> matAllocStr{};
  std::array<char, 32> instAllocStr{};

  formatBytes(memcpyStr.data(), sizeof(memcpyStr), memcpyBytes);

  formatBytes(stagingAllocStr.data(), sizeof(stagingAllocStr),
              stagingAllocBytes);
  formatBytes(stagingUsedStr.data(), sizeof(stagingUsedStr), stagingUsedBytes);

  formatBytes(bufStr.data(), sizeof(bufStr), bufBytes);
  formatBytes(bufAllocStr.data(), sizeof(bufAllocStr), bufAllocBytes);

  formatBytes(texStr.data(), sizeof(texStr), texBytes);
  formatBytes(texAllocStr.data(), sizeof(texAllocStr), texAllocBytes);

  formatBytes(matStr.data(), matStr.size(), matBytes);
  formatBytes(matAllocStr.data(), matAllocStr.size(), matAllocBytes);

  formatBytes(instStr.data(), sizeof(instStr), instBytes);
  formatBytes(instAllocStr.data(), sizeof(instAllocStr), instAllocBytes);

  // If prints show 0 here, its likely the scene if only static meshes
  std::array<char, 768> line{};
  ignore_snprintf(std::snprintf(
      line.data(), line.size(),
      "UPL: sub %-3llu  memcpy %-3llu/%s  staging used %s  inst "
      "%-3llu/%s  buf %-3llu/%s  tex %-3llu/%s  mat %-3llu/%s  "
      "alloc(staging %s c=%llu  buf %s  tex %s  mat %s  inst %s)",
      static_cast<unsigned long long>(submitCount),
      static_cast<unsigned long long>(memcpyCount), memcpyStr.data(),
      stagingUsedStr.data(), static_cast<unsigned long long>(instCount),
      instStr.data(), static_cast<unsigned long long>(bufCount), bufStr.data(),
      static_cast<unsigned long long>(texCount), texStr.data(),
      static_cast<unsigned long long>(matCount), matStr.data(),
      stagingAllocStr.data(),
      static_cast<unsigned long long>(stagingCreatedCount), bufAllocStr.data(),
      texAllocStr.data(), matAllocStr.data(), instAllocStr.data()));

  std::cerr << line.data() << "\n";
}

static void logGpu(const GpuProfiler::Frame &gpu) noexcept {
  if (!gpu.valid) {
    return;
  }

  std::array<char, 16> frame{};
  std::array<char, 16> main{};
  std::array<char, 16> idle{};

  formatMs(frame.data(), frame.size(), gpu.frameMs);
  formatMs(main.data(), main.size(), gpu.mainPassMs);
  formatMs(idle.data(), idle.size(), gpu.idleGapMs);

  std::array<char, 256> line{};
  ignore_snprintf(std::snprintf(line.data(), line.size(),
                                "GPU  ms: frame %s  main %s  idle %s",
                                frame.data(), main.data(), idle.data()));

  std::cerr << line.data() << "\n";
}

void logProfilerToConsole(const Telemetry &t) noexcept {

  // TODO: add rolling average over N frames and print that on N frame instead
  // of just N frame data
  CpuProfiler::Frame cpu{};
  UploadProfiler::Frame upl{};
  UploadProfiler::Frame uplLt{};
  GpuProfiler::Frame gpu{};
  if (!readPublished(t, cpu, upl, uplLt, gpu)) {
    return;
  }

  std::cerr << "\n[Profiler]\n";
  logCpuFrame(cpu);
  logUploadFrame(upl, uplLt);
  logGpu(gpu);
}

} // namespace profiling
#endif
