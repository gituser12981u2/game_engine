#pragma once

#if defined(ENABLE_TELEMETRY)

#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"

#include <chrono>

namespace profiling {

// Time-based publish
void setPublishPeriod(std::chrono::nanoseconds period) noexcept;

// Opportunistic publish
void publishMaybe() noexcept;

bool readPublished(const Telemetry &t, CpuProfiler::Frame &outCpu,
                   UploadProfiler::Frame &outUpload) noexcept;

} // namespace profiling
#endif
