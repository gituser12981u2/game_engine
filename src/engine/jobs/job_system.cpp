#include "engine/jobs/job_system.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

thread_local uint32_t JobSystem::s_tlsWorkerIndex =
    JobSystem::kInvalidWorkerIndex;

uint32_t JobSystem::currentWorkerIndex() noexcept { return s_tlsWorkerIndex; }

bool JobSystem::init(uint32_t threadCount) {
  shutdown();

  // m_hooks = hooks;

  uint32_t hc = std::max(1U, std::thread::hardware_concurrency());
  if (threadCount == 0) {
    threadCount = hc;
  }
  threadCount = std::max(1U, threadCount);

  m_threadCount = threadCount;
  m_stop.store(false, std::memory_order_relaxed);
  m_running.store(true, std::memory_order_release);

#if defined(ENABLE_TELEMETRY)
  m_workerTelemetry.clear();
  m_workerTelemetry.reserve(threadCount);
  for (uint32_t i = 0; i < threadCount; ++i) {
    m_workerTelemetry.emplace_back(std::make_unique<profiling::Telemetry>());
  }
#endif

  m_workers.reserve(threadCount);
  for (uint32_t i = 0; i < threadCount; ++i) {
    m_workers.emplace_back([this, i] { workerMain(i); });
  }

  return true;
}

void JobSystem::shutdown() noexcept {
  if (!m_running.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  m_stop.store(true, std::memory_order_release);

  // Wake all workers to observe stop
  m_queueCv.notify_all();
  for (std::thread &thread : m_workers) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  m_workers.clear();

  // Drop remaining jobs
  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.clear();
  }

  m_threadCount = 0;
  m_stop.store(false, std::memory_order_relaxed);
}

void JobSystem::enqueue(JobFn fn) {
  if (!fn) {
    return;
  }

  // Execute inline to avoid silent losses
  if (!m_running.load(std::memory_order_acquire)) {
    fn();
    return;
  }

  m_jobsSubmitted.fetch_add(1, std::memory_order_relaxed);

  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push_back(Job{std::move(fn)});
  }
  m_queueCv.notify_one();
}

void JobSystem::Group::enqueue(JobFn fn) {
  if (!fn || m_sys == nullptr) {
    return;
  }

  m_pending.fetch_add(1, std::memory_order_relaxed);

  m_sys->enqueue([this, f = std::move(fn)]() mutable {
    f();

    if (m_pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      std::lock_guard<std::mutex> lock(m_waitMutex);
      m_waitCv.notify_all();
    }
  });
}

void JobSystem::Group::wait() {
  if (m_pending.load(std::memory_order_acquire) == 0) {
    return;
  }

  std::unique_lock<std::mutex> lock(m_waitMutex);
  m_waitCv.wait(
      lock, [this] { return m_pending.load(std::memory_order_acquire) == 0; });
}

bool JobSystem::popJob(Job &out) {
  std::unique_lock<std::mutex> lock(m_queueMutex);

  m_queueCv.wait(lock, [this] {
    return m_stop.load(std::memory_order_acquire) || !m_queue.empty();
  });

  if (m_stop.load(std::memory_order_acquire)) {
    return false;
  }

  if (m_queue.empty()) {
    return false;
  }

  out = std::move(m_queue.front());
  m_queue.pop_front();
  return true;
}

void JobSystem::workerMain(uint32_t workerIndex) {
  s_tlsWorkerIndex = workerIndex;

#if defined(ENABLE_TELEMETRY)
  profiling::setTlsTelemetry(m_workerTelemetry[workerIndex].get());
#endif

  if (m_hooks.onWorkerStart != nullptr) {
    m_hooks.onWorkerStart(workerIndex);
  }

  for (;;) {
    if (m_stop.load(std::memory_order_acquire)) {
      return;
    }

    Job job{};
    if (!popJob(job)) {
      if (m_stop.load(std::memory_order_acquire)) {
        return;
      }
      continue;
    }

    if (job.fn) {
      job.fn();
      m_jobsExecuted.fetch_add(1, std::memory_order_release);
    }
  }

  if (m_hooks.onWorkerStop != nullptr) {
    m_hooks.onWorkerStop(workerIndex);
  }

#if defined(ENABLE_TELEMETRY)
  profiling::setTlsTelemetry(nullptr);
#endif

  s_tlsWorkerIndex = kInvalidWorkerIndex;
}
