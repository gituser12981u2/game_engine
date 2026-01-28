#pragma once

#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/gpu_frame_stats.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"

namespace profiling {

struct ProfilerSnapshot {
  bool cpuValid = false;
  bool uploadValid = false;
  bool gpuValid = false;

  CpuProfiler::Frame cpu{};
  UploadProfiler::Frame uploadFrame{};
  UploadProfiler::Frame uploadLifetime{};
  GpuProfiler::Frame gpu{};
};

bool buildSnapshotFromTls(ProfilerSnapshot &out) noexcept;

} // namespace profiling
