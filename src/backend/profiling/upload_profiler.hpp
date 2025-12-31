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
    StagingCreatedBytes,

    BufferUploadCount,
    BufferUploadBytes,

    TextureUploadCount,
    TextureUploadBytes,

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
    case Stat::StagingCreatedBytes:
      return "StagingCreatedBytes";
    case Stat::BufferUploadCount:
      return "BufferUploadCount";
    case Stat::BufferUploadBytes:
      return "BufferUploadBytes";
    case Stat::TextureUploadCount:
      return "TextureUploadCount";
    case Stat::TextureUploadBytes:
      return "TextureUploadBytes";
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
