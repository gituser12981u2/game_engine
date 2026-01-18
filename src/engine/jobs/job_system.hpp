#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#if defined(ENABLE_TELEMETRY)
#include "backend/profiling/telemetry/telemetry.hpp"
#endif

class JobSystem {
public:
  struct Hooks {
    void (*onWorkerStart)(uint32_t workerIndex) = nullptr;
    void (*onWorkerStop)(uint32_t workerIndex) = nullptr;
  };

  using JobFn = std::function<void()>;

  static constexpr uint32_t kInvalidWorkerIndex = 0xFFFF'FFFFU;

  JobSystem() = default;
  ~JobSystem() noexcept { shutdown(); }

  JobSystem(const JobSystem &) = delete;
  JobSystem &operator=(const JobSystem &) = delete;

  JobSystem(JobSystem &&) = delete;
  JobSystem &operator=(JobSystem &&) = delete;

  bool init(uint32_t threadCount = 0);
  // bool init(uint32_t threadCount = 0, Hooks hooks);
  void shutdown() noexcept;

  [[nodiscard]] bool initialized() const noexcept { return m_running.load(); }
  [[nodiscard]] uint32_t threadCount() const noexcept { return m_threadCount; }

  // TLS worker index for the calling thread
  // - Worker threads crated by this JobSystem: 0..threadCount-1
  // - Any other thread (main thread, foreign thread): kInvalidWorkerIndex
  [[nodiscard]] static uint32_t currentWorkerIndex() noexcept;

  void enqueue(JobFn fn);

  // A join point for a set of jobs
  class Group {
  public:
    Group() = default;
    ~Group() noexcept = default;

    Group(const Group &) = delete;
    Group &operator=(const Group &) = delete;

    Group(Group &&) noexcept = delete;
    Group &operator=(Group &&) noexcept = delete;

    // increments pending
    void enqueue(JobFn fn);

    // waits until pending == 0
    void wait();

    [[nodiscard]] uint32_t pending() const noexcept {
      return m_pending.load(std::memory_order_relaxed);
    }

  private:
    friend class JobSystem;
    explicit Group(JobSystem *sys) : m_sys(sys) {}

    JobSystem *m_sys = nullptr;
    std::atomic<uint32_t> m_pending{0};
    std::mutex m_waitMutex;
    std::condition_variable m_waitCv;
  };

  [[nodiscard]] Group makeGroup() { return Group(this); }

  // Splits [0, count) into chunks of size grain.
  template <class F> void parallel_for(uint32_t count, uint32_t grain, F &&fn) {
    if (count == 0) {
      return;
    }

    if (grain == 0) {
      grain = 1;
    }

    auto group = makeGroup();
    for (uint32_t start = 0; start < count; start += grain) {
      uint32_t end = (start + grain < count) ? (start + grain) : count;
      group.enqueue([start, end, func = std::forward<F>(fn)]() mutable {
        for (uint32_t i = start; i < end; ++i) {
          func(i);
        }
      });
    }

    group.wait();
  }

  [[nodiscard]] uint64_t jobsSubmitted() const noexcept {
    return m_jobsSubmitted.load(std::memory_order_relaxed);
  }

  [[nodiscard]] uint64_t jobsExecuted() const noexcept {
    return m_jobsExecuted.load(std::memory_order_relaxed);
  }

private:
  Hooks m_hooks{};

  struct Job {
    JobFn fn;
  };

  bool popJob(Job &out);
  void workerMain(uint32_t workerIndex);

  // Queue
  std::mutex m_queueMutex;
  std::condition_variable m_queueCv;
  std::deque<Job> m_queue;

  std::vector<std::thread> m_workers;
  uint32_t m_threadCount = 0;

  std::atomic<bool> m_running{false};
  std::atomic<bool> m_stop{false};

  // Debug counters
  std::atomic<uint64_t> m_jobsSubmitted{0};
  std::atomic<uint64_t> m_jobsExecuted{0};

  static thread_local uint32_t s_tlsWorkerIndex;

#if defined(ENABLE_TELEMETRY)
  std::vector<std::unique_ptr<profiling::Telemetry>> m_workerTelemetry;
#endif
};
