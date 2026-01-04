#include "backend/profiling/upload_profiler.hpp"

#include <cstddef>

static constexpr bool isLifetimeStat(UploadProfiler::Stat stat) noexcept {
  using S = UploadProfiler::Stat;
  switch (stat) {
  case S::BufferAllocatedBytes:
  case S::TextureAllocatedBytes:
  case S::MaterialAllocatedBytes:
  case S::InstanceAllocatedBytes:
  case S::StagingCreatedCount:
  case S::StagingAllocatedBytes:
    return true;
  default:
    return false;
  }
}

void UploadProfiler::beginFrame() noexcept { resetFrame(); }

void UploadProfiler::endFrame() noexcept {
  m_lastFrame = m_frame;
  resetFrame();
}

void UploadProfiler::add(Stat stat, std::uint64_t value) noexcept {
  const size_t i = static_cast<size_t>(stat);
  m_frame.v[i] += value;

  if (isLifetimeStat(stat)) {
    m_lifetime.v[i] += value;
  }
}
