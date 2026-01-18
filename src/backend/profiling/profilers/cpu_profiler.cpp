#include "backend/profiling/profilers/cpu_profiler.hpp"

CpuProfiler::Scope::Scope(CpuProfiler &profiler, Stat stat) noexcept
    : m_profiler(&profiler), m_stat(stat), m_t0(clock::now()) {}

CpuProfiler::Scope::~Scope() noexcept { end(); }

CpuProfiler::Scope &CpuProfiler::Scope::operator=(Scope &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  end();

  m_profiler = other.m_profiler;
  m_stat = other.m_stat;
  m_t0 = other.m_t0;
  other.m_profiler = nullptr;

  return *this;
}

void CpuProfiler::Scope::end() noexcept {
  if (m_profiler == nullptr) {
    return;
  }

  const auto t1 = clock::now();
  const double ms =
      std::chrono::duration<double, std::milli>(t1 - m_t0).count();

  m_profiler->addMs(m_stat, ms);
  m_profiler = nullptr;
}

void CpuProfiler::beginInterval() noexcept { m_cur = Frame{}; }

void CpuProfiler::endInterval() noexcept {
  finalizeOther();
  m_last = m_cur;
  resetCurrent();
}

void CpuProfiler::addMs(Stat stat, double ms) noexcept {
  m_cur.ms[static_cast<size_t>(stat)] += ms;
}

void CpuProfiler::finalizeOther() noexcept {
  const auto idx = [](Stat stat) { return static_cast<size_t>(stat); };

  const double frame = m_cur.ms[idx(Stat::FrameTotal)];

  const double accounted =
      m_cur.ms[idx(Stat::Acquire)] + m_cur.ms[idx(Stat::WaitForFence)] +
      m_cur.ms[idx(Stat::UpdatePerFrameUBO)] + m_cur.ms[idx(Stat::RecordCmd)] +
      m_cur.ms[idx(Stat::QueuePresent)] + m_cur.ms[idx(Stat::QueueSubmit)] +
      m_cur.ms[idx(Stat::SwapchainRecreate)];

  m_cur.ms[idx(Stat::Other)] = std::max(0.0, frame - accounted);
}
