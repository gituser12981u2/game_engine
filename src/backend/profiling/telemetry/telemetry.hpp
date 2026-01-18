#pragma once

namespace profiling {

struct Telemetry;

extern thread_local Telemetry *g_tls;

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

#define GE_JOIN_IMPL(a, b) a##b
#define GE_JOIN(a, b) GE_JOIN_IMPL(a, b)

#if defined(ENABLE_TELEMETRY)

#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include <atomic>
#include <chrono>

namespace profiling {

struct alignas(64) PublishedTelemetry {
  std::atomic<uint32_t> seq{0}; // even=stable, odd=writer in progress
  CpuProfiler::Frame cpu{};
  UploadProfiler::Frame upload{};
};

struct Telemetry {
  Telemetry() = default;
  ~Telemetry() = default;

  Telemetry(const Telemetry &) = delete;
  Telemetry &operator=(const Telemetry &) = delete;
  Telemetry(Telemetry &&) = delete;
  Telemetry &operator=(Telemetry &&) = delete;

  CpuProfiler cpu;
  UploadProfiler upload;

  PublishedTelemetry published{};
  std::chrono::steady_clock::time_point lastPublish;
};

inline CpuProfiler *cpuPtr() noexcept {
  Telemetry *t = telemetry();
  if (t == nullptr) {
    return nullptr;
  }
  return &t->cpu;
}

inline UploadProfiler *uploadPtr() noexcept {
  Telemetry *t = telemetry();
  if (t == nullptr) {
    return nullptr;
  }
  return &t->upload;
}

class CpuScope {
public:
  CpuScope(CpuProfiler *cpu, CpuProfiler::Stat stat) noexcept {
    if (cpu != nullptr) {
      m_scope.emplace(*cpu, stat);
    }
  }

private:
  std::optional<CpuProfiler::Scope> m_scope;
};

} // namespace profiling

#define PROFILE_CPU_SCOPE(stat_enum)                                           \
  ::profiling::CpuScope GE_JOIN(_ge_cpu_scope_, __COUNTER__)(                  \
      ::profiling::cpuPtr(), (stat_enum))

#define PROFILE_CPU_INC_DRAW_CALLS(n)                                          \
  do {                                                                         \
    if (auto *p = ::profiling::cpuPtr()) {                                     \
      p->incDrawCalls((n));                                                    \
    }                                                                          \
  } while (0)
#define PROFILE_CPU_ADD_TRIANGLES(n)                                           \
  do {                                                                         \
    if (auto *p = ::profiling::cpuPtr()) {                                     \
      p->addTriangles((n));                                                    \
    }                                                                          \
  } while (0)

#define PROFILE_CPU_INC_PIPELINE_BINDS(n)                                      \
  do {                                                                         \
    if (auto *p = ::profiling::cpuPtr()) {                                     \
      p->incPipelineBinds((n));                                                \
    }                                                                          \
  } while (0)

#define PROFILE_CPU_INC_DESCRIPTOR_BINDS(n)                                    \
  do {                                                                         \
    if (auto *p = ::profiling::cpuPtr()) {                                     \
      p->incDescriptorBinds((n));                                              \
    }                                                                          \
  } while (0)

#define PROFILE_CPU_ADD_INSTANCES(n)                                           \
  do {                                                                         \
    if (auto *p = ::profiling::cpuPtr()) {                                     \
      p->addInstances((n));                                                    \
    }                                                                          \
  } while (0)

#define PROFILE_UPLOAD_ADD(stat_enum, value_u64)                               \
  profilerAdd(::profiling::uploadPtr(), (stat_enum), (std::uint64_t)(value_u64))

#define PROFILE_UPLOAD_INC(stat_enum) PROFILE_UPLOAD_ADD((stat_enum), 1)

#else

#define PROFILE_CPU_SCOPE(stat_enum) ((void)0)

#define PROFILE_CPU_INC_DRAW_CALLS(n) ((void)0)
#define PROFILE_CPU_ADD_TRIANGLES(n) ((void)0)
#define PROFILE_CPU_INC_PIPELINE_BINDS(n) ((void)0)
#define PROFILE_CPU_INC_DESCRIPTOR_BINDS(n) ((void)0)
#define PROFILE_CPU_ADD_INSTANCES(n) ((void)0)

#define PROFILE_UPLOAD_ADD(stat_enum, value_u64) ((void)0)
#define PROFILE_UPLOAD_INC(stat_enum) ((void)0)

#endif
