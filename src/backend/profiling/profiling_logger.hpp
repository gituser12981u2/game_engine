#pragma once

#include "backend/profiling/cpu_profiler.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/profiling/vk_gpu_profiler.hpp"

#include <chrono>
#include <ratio>
#include <string_view>
#include <sys/wait.h>

namespace profiling {

// Per-frame logging
class FrameLogger {
public:
  void setPeriod(uint64_t n) noexcept { m_period = n; }
  void logPerFrame(const CpuProfiler &cpu, const VkGpuProfiler &gpu,
                   const UploadProfiler &upload) noexcept;

private:
  bool shouldLog() noexcept;

  uint64_t m_frameCounter = 0;
  uint64_t m_period = 120;
};

// One-off event logging
enum class Event : std::uint8_t {
  DeviceWaitIdle = 0,
  SwapchainRecreate,
  Count
};

static constexpr std::string_view name(Event e) noexcept {
  switch (e) {
  case Event::DeviceWaitIdle:
    return "vkDeviceWaitIdle";
  case Event::SwapchainRecreate:
    return "SwapchainRecreate";
  default:
    return "Unknown";
  }
}

#ifndef NDEBUG

void emit(Event e, double ms) noexcept;

class EventScope {
public:
  EventScope(Event e) noexcept : m_event(e), m_t0(clock::now()) {}
  ~EventScope() noexcept { end(); }

  EventScope(const EventScope &) = delete;
  EventScope &operator=(const EventScope &) = delete;
  EventScope(EventScope &&) = delete;
  EventScope &operator=(EventScope &&) = delete;

private:
  using clock = std::chrono::steady_clock;

  void end() noexcept {
    const auto t1 = clock::now();
    const double ms =
        std::chrono::duration<double, std::milli>(t1 - m_t0).count();
    emit(m_event, ms);
  }

  Event m_event{};
  clock::time_point m_t0;
};

#else

// Release build
inline void emit(Event, double) noexcept {}

class EventScope {
public:
  explicit EventScope(Event) noexcept {}
};

#endif

} // namespace profiling
