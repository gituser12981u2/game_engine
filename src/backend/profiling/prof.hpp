#pragma once

namespace profiling {

struct Telemetry;

Telemetry *tlsTelemetry() noexcept;
void setTlsTelemetry(Telemetry *t) noexcept;

inline Telemetry *telemetry() noexcept { return tlsTelemetry(); }

} // namespace profiling

#if defined(TRACY_ENABLE)
#define PROF_FRAME() FrameMark
#define PROF_FRAME_N(name_literal) FrameMarkNamed(name_literal)

#else
#define PROF_FRAME() ((void)0)
#define PROF_FRAME_N(name_literal) ((void)0)

#define PROF_SCOPE() ((void)0)
#define PROF_SCOPE_N(name_literal) ((void)0)

#define PROF_THREAD_NAME(name_literal) ((void)0)

#endif

#if defined(GE_PROF_TELEMETRY)

#include "backend/profiling/cpu_profiler.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/profiling/vk_gpu_profiler.hpp"

namespace profiling {

struct Telemetry {
  CpuProfiler cpu;
  UploadProfiler upload;
  VkGpuProfiler gpu;
};

inline CpuProfiler &cpu() noexcept { return telemetry()->cpu; }
inline UploadProfiler &upload() noexcept { return telemetry()->upload; }
inline VkGpuProfiler &gpu() noexcept { return telemetry()->gpu; }

} // namespace profiling

#define PROF_CPU_SCOPE(stat_enum)                                              \
  ::CpuProfiler::Scope _prof_cpu_scope_##__LINE__(::profiling::cpu(),          \
                                                  (stat_enum))

#define PROF_UPLOAD_ADD(stat_enum, value_u64)                                  \
  profilerAdd(::profiling::uploadPtr(), (stat_enum), (std::uint64_t)(value_u64))

#define PROF_UPLOAD_INC(stat_enum) PROF_UPLOAD_ADD((stat_enum), 1)

#define PROF_CPU_INC_DRAWS(n) (::profiling::cpu().incDrawCalls((n)))
#define PROF_CPU_ADD_TRIS(n) (::profiling::cpu().addTriangles((n)))
#define PROF_CPU_INC_PIPE(n) (::profiling::cpu().incPipelineBinds((n)))
#define PROF_CPU_INC_DESC(n) (::profiling::cpu().incDescriptorBinds((n)))
#define PROF_CPU_ADD_INST(n) (::profiling::cpu().addInstances((n)))

#else
#define PROF_CPU_SCOPE(stat_enum) ((void)0)

#define PROF_UPLOAD_ADD(stat_enum, value_u64) ((void)0)
#define PROF_UPLOAD_INC(stat_enum) ((void)0)

#define PROF_CPU_INC_DRAWS(n) ((void)0)
#define PROF_CPU_ADD_TRIS(n) ((void)0)
#define PROF_CPU_INC_PIPE(n) ((void)0)
#define PROF_CPU_INC_DESC(n) ((void)0)
#define PROF_CPU_ADD_INST(n) ((void)0)
#endif
