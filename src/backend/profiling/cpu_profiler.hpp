#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string_view>

class CpuProfiler {
public:
  enum class Stat : uint8_t {
    FrameTotal = 0,
    Acquire,
    WaitForFence,
    UpdatePerFrameUBO,
    RecordCmd,
    QueueSubmit,
    QueuePresent,
    SwapchainRecreate,
    WaitIdle,
    Other,
    Count
  };

  struct FrameStats {
    std::array<double, static_cast<size_t>(Stat::Count)> ms{};
    uint32_t drawCalls = 0;
    uint64_t triangles = 0;
    uint32_t pipelineBinds = 0;
    uint32_t descriptorBinds = 0;
  };

  class Scope {
  public:
    Scope(CpuProfiler &profiler, Stat stat) noexcept
        : m_profiler(&profiler), m_stat(stat), m_t0(clock::now()) {}
    ~Scope() noexcept { end(); }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

    Scope(Scope &&) = delete;
    Scope &operator=(Scope &&other) noexcept {
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

  private:
    using clock = std::chrono::steady_clock;

    void end() noexcept {
      if (m_profiler == nullptr) {
        return;
      }

      const auto t1 = clock::now();
      const double ms =
          std::chrono::duration<double, std::milli>(t1 - m_t0).count();

      m_profiler->add(m_stat, ms);
      m_profiler = nullptr;
    }

    CpuProfiler *m_profiler = nullptr;
    Stat m_stat{};
    clock::time_point m_t0;
  };

  void endFrame() noexcept {
    const auto idx = [](Stat stat) { return static_cast<size_t>(stat); };

    const double frame = m_cur.ms[idx(Stat::FrameTotal)];

    const double accounted =
        m_cur.ms[idx(Stat::Acquire)] + m_cur.ms[idx(Stat::WaitForFence)] +
        m_cur.ms[idx(Stat::UpdatePerFrameUBO)] +
        m_cur.ms[idx(Stat::RecordCmd)] + m_cur.ms[idx(Stat::QueuePresent)] +
        m_cur.ms[idx(Stat::QueueSubmit)] +
        m_cur.ms[idx(Stat::SwapchainRecreate)];

    m_cur.ms[idx(Stat::Other)] = std::max(0.0, frame - accounted);

    m_last = m_cur;
    resetCurrent();
  }

  // Counters
  void incDrawCalls(uint32_t n = 1) noexcept { m_cur.drawCalls += n; }
  void addTriangles(uint64_t n) noexcept { m_cur.triangles += n; }
  void incPipelineBinds(uint32_t n = 1) noexcept { m_cur.pipelineBinds += n; }
  void incDescriptorBinds(uint32_t n = 1) noexcept {
    m_cur.descriptorBinds += n;
  }

  [[nodiscard]] const FrameStats &last() const noexcept { return m_last; }

  // Printing / UI
  static constexpr std::string_view name(Stat stat) noexcept {
    switch (stat) {
    case Stat::FrameTotal:
      return "FrameTotal";
    case Stat::Acquire:
      return "Acquire";
    case Stat::WaitForFence:
      return "WaitForFence";
    case Stat::UpdatePerFrameUBO:
      return "UpdatePerFrameUBO";
    case Stat::RecordCmd:
      return "RecordCmd";
    case Stat::QueueSubmit:
      return "QueueSubmit";
    case Stat::QueuePresent:
      return "QueuePresent";
    case Stat::SwapchainRecreate:
      return "SwapchainRecreate";
    case Stat::Other:
      return "Other";
    default:
      return "Unknown";
    }
  }

private:
  friend class Scope;

  void add(Stat stat, double ms) noexcept {
    m_cur.ms[static_cast<size_t>(stat)] += ms;
  }

  void resetCurrent() noexcept { m_cur = FrameStats{}; }

  FrameStats m_cur{};
  FrameStats m_last{};
};
