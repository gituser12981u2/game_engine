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

    MaterialUploadCount,
    MaterialUploadBytes,
    MaterialAllocatedBytes,

    InstanceUploadCount,
    InstanceUploadBytes,
    InstanceAllocatedBytes,

    Count
  };

  struct Stats {
    std::array<std::uint64_t, static_cast<size_t>(Stat::Count)> v{};
  };

  void beginFrame() noexcept;
  void endFrame() noexcept;

  [[nodiscard]] const Stats &last() const noexcept { return m_lastFrame; }
  [[nodiscard]] const Stats &lifetime() const noexcept { return m_lifetime; }

  void add(Stat stat, std::uint64_t value) noexcept;

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

    case Stat::MaterialUploadCount:
      return "MaterialUploadCount";
    case Stat::MaterialUploadBytes:
      return "MaterialUploadBytes";
    case Stat::MaterialAllocatedBytes:
      return "MaterialAllocatedBytes";

    case Stat::InstanceUploadCount:
      return "InstanceUploadCount";
    case Stat::InstanceUploadBytes:
      return "InstanceUploadBytes";
    case Stat::InstanceAllocatedBytes:
      return "InstanceAllocatedBytes";
    default:
      return "Unknown";
    }
  }

private:
  void resetFrame() noexcept { m_frame = Stats{}; }

  Stats m_frame{};
  Stats m_lastFrame{};
  Stats m_lifetime{};
};

static inline void profilerAdd(UploadProfiler *profiler,
                               UploadProfiler::Stat stat,
                               std::uint64_t v) noexcept {
  if (profiler != nullptr) {
    profiler->add(stat, v);
  }
}
