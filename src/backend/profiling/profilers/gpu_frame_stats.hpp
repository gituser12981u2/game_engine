#pragma once

namespace GpuProfiler {

struct Frame {
  bool valid = false;
  double frameMs = 0.0;
  double mainPassMs = 0.0;
  double idleGapMs = 0.0;
};

} // namespace GpuProfiler
