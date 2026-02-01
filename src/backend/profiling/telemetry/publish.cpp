#include "backend/profiling/telemetry/publish.hpp"

#if defined(ENABLE_TELEMETRY)

#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/gpu_frame_stats.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>

namespace profiling {

static std::atomic<int64_t> g_publishPeriodNs{1'000'000}; // 1ms

void setPublishPeriod(std::chrono::nanoseconds period) noexcept {
  const int64_t ns = period.count();
  g_publishPeriodNs.store(ns > 0 ? ns : 0, std::memory_order_relaxed);
}

static inline std::chrono::nanoseconds publishPeriod() noexcept {
  return std::chrono::nanoseconds{
      g_publishPeriodNs.load(std::memory_order_relaxed)};
}

static inline void publishSeqLock(PublishedTelemetry &p,
                                  const CpuProfiler::Frame &cpu,
                                  const UploadProfiler::Frame &upl,
                                  const UploadProfiler::Frame &uplLifetime,
                                  const GpuProfiler::Frame &gpu) noexcept {
  p.seq.fetch_add(1, std::memory_order_relaxed); // odd
  std::atomic_thread_fence(std::memory_order_release);

  p.cpu = cpu;
  p.upload = upl;
  p.uploadLifetime = uplLifetime;
  p.gpu = gpu;

  std::atomic_thread_fence(std::memory_order_release);
  p.seq.fetch_add(1, std::memory_order_relaxed); // even
}

bool publishMaybe(const GpuProfiler::Frame &gpu) noexcept {
  Telemetry *t = telemetry();
  if (t == nullptr) {
    return false;
  }

  const auto period = publishPeriod();
  if (period.count() <= 0) {
    return false; // publishing disabled
  }

  const auto now = std::chrono::steady_clock::now();

  if (t->lastPublish.time_since_epoch().count() == 0) {
    publishSeqLock(t->published, t->cpu.last(), t->upload.last(),
                   t->upload.lifetime(), gpu);
    t->lastPublish = now;
    return true;
  }

  if ((now - t->lastPublish) < period) {
    return false;
  }

  publishSeqLock(t->published, t->cpu.last(), t->upload.last(),
                 t->upload.lifetime(), gpu);
  t->lastPublish = now;
  return true;
}

bool readPublished(const Telemetry &t, CpuProfiler::Frame &outCpu,
                   UploadProfiler::Frame &outUpload,
                   UploadProfiler::Frame &outUploadLifetime,
                   GpuProfiler::Frame &outGpu) noexcept {
  const PublishedTelemetry &p = t.published;

  for (int tries = 0; tries < 4; ++tries) {
    const uint32_t a = p.seq.load(std::memory_order_acquire);
    if ((a & 1U) != 0U) {
      continue;
    }

    outCpu = p.cpu;
    outUpload = p.upload;
    outUploadLifetime = p.uploadLifetime;
    outGpu = p.gpu;

    std::atomic_thread_fence(std::memory_order_acquire);
    const uint32_t b = p.seq.load(std::memory_order_acquire);
    if (a == b) {
      return true;
    }
  }

  return false;
}

} // namespace profiling

#endif
