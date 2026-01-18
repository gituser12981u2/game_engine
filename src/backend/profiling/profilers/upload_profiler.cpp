#include "backend/profiling/profilers/upload_profiler.hpp"

#include <cstddef>

bool UploadProfiler::isLifetimeStat(Stat stat) noexcept {
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

void UploadProfiler::beginInterval() noexcept { resetCur(); }

void UploadProfiler::endInterval() noexcept {
  m_lastFrame = m_cur;
  resetCur();
}

void UploadProfiler::add(Stat stat, std::uint64_t value) noexcept {
  const size_t i = static_cast<size_t>(stat);
  m_cur.v[i] += value;

  if (isLifetimeStat(stat)) {
    m_lifetime.v[i] += value;
  }
}
