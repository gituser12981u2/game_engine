#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string_view>

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

    Count
  };

  struct FrameStats {
    std::array<std::uint64_t, static_cast<size_t>(Stat::Count)> v{};
  };

  void endFrame() noexcept {
    m_last = m_cur;
    m_cur = FrameStats{};
  }
  [[nodiscard]] const FrameStats &last() const noexcept { return m_last; }

  void add(Stat stat, std::uint64_t n = 1) noexcept {
    m_cur.v[static_cast<std::size_t>(stat)] += n;
  }

  static constexpr std::string_view name(Stat stat) noexcept {
    switch (stat) {
    case Stat::UploadSubmitCount:
      return "UploadSubmitCount";
    case Stat::UploadMemcpyCount:
      return "UploadMemcpyCount";
    case Stat::UploadMemcpyBytes:
      return "UploadMemcpyBytes";
    case Stat::StagingCreatedCount:
      return "StagingCreatedCount";
    case Stat::StagingUsedBytes:
      return "StagingUsedBytes";
    case Stat::StagingAllocatedBytes:
      return "StatingAllocatedBytes";
    case Stat::BufferUploadCount:
      return "BufferUploadCount";
    case Stat::BufferUploadBytes:
      return "BufferUploadBytes";
    case Stat::BufferAllocatedBytes:
      return "StatingAllocatedBytes";
    case Stat::TextureUploadCount:
      return "TextureUploadCount";
    case Stat::TextureUploadBytes:
      return "TextureUploadBytes";
    case Stat::TextureAllocatedBytes:
      return "TextureAllocatedBytes";
    default:
      return "Unknown";
    }
  }

private:
  FrameStats m_cur{};
  FrameStats m_last{};
};

static inline void profilerAdd(UploadProfiler *profiler,
                               UploadProfiler::Stat stat,
                               std::uint64_t n) noexcept {
  if (profiler != nullptr) {
    profiler->add(stat, n);
  }
}
