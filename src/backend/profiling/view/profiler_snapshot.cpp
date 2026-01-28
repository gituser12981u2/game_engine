#include "backend/profiling/view/profiler_snapshot.hpp"

#if defined(ENABLE_TELEMETRY)
#include "backend/profiling/profilers/cpu_profiler.hpp"
#include "backend/profiling/profilers/gpu_frame_stats.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include "backend/profiling/telemetry/publish.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#endif

namespace profiling {

bool buildSnapshotFromTls(ProfilerSnapshot &out) noexcept {
  out = {};

#if defined(ENABLE_TELEMETRY)
  Telemetry *t = telemetry();
  if (t != nullptr) {
    CpuProfiler::Frame cpu{};
    UploadProfiler::Frame upl{};
    UploadProfiler::Frame uplLt{};
    GpuProfiler::Frame gpu{};
    if (readPublished(*t, cpu, upl, uplLt, gpu)) {
      out.cpu = cpu;
      out.uploadFrame = upl;
      out.uploadLifetime = t->upload.lifetime();
      out.gpu = gpu;
      out.cpuValid = true;
      out.uploadValid = true;
      out.gpuValid = gpu.valid;
      return true;
    }
  }
#endif

  return false;
}

} // namespace profiling
