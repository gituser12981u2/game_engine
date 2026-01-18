#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>

class UploadProfiler {
public:
  enum class Stat : uint8_t {
    UploadSubmitCount = 0,

    UploadMemcpyCount,
    UploadMemcpyBytes,

    StagingCreatedCount,
    StagingAllocatedBytes,
    StagingUsedBytes,

    BufferUploadCount,
    BufferUploadBytes,
    BufferAllocatedBytes,

    TextureUploadCount,
    TextureUploadBytes,
    TextureAllocatedBytes,

    MaterialUploadCount,
    MaterialUploadBytes,
    MaterialAllocatedBytes,

    InstanceUploadCount,
    InstanceUploadBytes,
    InstanceAllocatedBytes,

    Count
  };

  struct Frame {
    std::array<std::uint64_t, static_cast<size_t>(Stat::Count)> v{};
  };

  void beginInterval() noexcept;
  void endInterval() noexcept;

  [[nodiscard]] const Frame &cur() const noexcept { return m_cur; }
  [[nodiscard]] const Frame &last() const noexcept { return m_lastFrame; }
  [[nodiscard]] const Frame &lifetime() const noexcept { return m_lifetime; }

  void add(Stat stat, std::uint64_t value) noexcept;

private:
  static bool isLifetimeStat(Stat stat) noexcept;

  void resetCur() noexcept { m_cur = Frame{}; }

  Frame m_cur{};
  Frame m_lastFrame{};
  Frame m_lifetime{};
};

inline UploadProfiler::Frame &
operator+=(UploadProfiler::Frame &a, const UploadProfiler::Frame &b) noexcept {
  for (size_t i = 0; i < a.v.size(); ++i) {
    a.v[i] += b.v[i];
  }

  return a;
}

static inline void profilerAdd(UploadProfiler *profiler,
                               UploadProfiler::Stat stat,
                               std::uint64_t v) noexcept {
  if (profiler != nullptr) {
    profiler->add(stat, v);
  }
}
