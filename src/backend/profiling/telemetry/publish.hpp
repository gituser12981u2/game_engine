#pragma once

#if defined(ENABLE_TELEMETRY)

#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"

#include <chrono>

namespace profiling {

void setPublishPeriod(std::chrono::nanoseconds period) noexcept;

// Opportunistic publish
bool publishMaybe(const GpuProfiler::Frame &gpu) noexcept;

bool readPublished(const Telemetry &t, CpuProfiler::Frame &outCpu,
                   UploadProfiler::Frame &outUpload,
                   UploadProfiler::Frame &outUploadLifetime,
                   GpuProfiler::Frame &outGpu) noexcept;

} // namespace profiling
#endif
