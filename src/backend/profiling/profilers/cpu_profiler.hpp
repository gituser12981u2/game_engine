#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>

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

  struct Frame {
    std::array<double, static_cast<size_t>(Stat::Count)> ms{};
    uint32_t drawCalls = 0;
    uint64_t triangles = 0;
    uint32_t pipelineBinds = 0;
    uint32_t descriptorBinds = 0;
    uint32_t instances = 0;
  };

  class Scope {
  public:
    using clock = std::chrono::steady_clock;

    Scope(CpuProfiler &profiler, Stat stat) noexcept;
    ~Scope() noexcept;

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

    Scope(Scope &&) = delete;
    Scope &operator=(Scope &&other) noexcept;

  private:
    void end() noexcept;

    CpuProfiler *m_profiler = nullptr;
    Stat m_stat{};
    clock::time_point m_t0;
  };

  void beginInterval() noexcept;
  void endInterval() noexcept;

  void incDrawCalls(uint32_t n = 1) noexcept { m_cur.drawCalls += n; }
  void addTriangles(uint64_t n) noexcept { m_cur.triangles += n; }
  void addInstances(uint32_t n) noexcept { m_cur.instances += n; }
  void incPipelineBinds(uint32_t n = 1) noexcept { m_cur.pipelineBinds += n; }
  void incDescriptorBinds(uint32_t n = 1) noexcept {
    m_cur.descriptorBinds += n;
  }

  [[nodiscard]] const Frame &cur() const noexcept { return m_cur; }
  [[nodiscard]] const Frame &last() const noexcept { return m_last; }

private:
  friend class Scope;

  void addMs(Stat stat, double ms) noexcept;
  void resetCurrent() noexcept { m_cur = Frame{}; }
  void finalizeOther() noexcept;

  Frame m_cur{};
  Frame m_last{};
};
