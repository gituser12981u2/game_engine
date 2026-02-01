#pragma once

#if defined(ENABLE_TELEMETRY)
#include "backend/profiling/telemetry/telemetry.hpp"

namespace profiling {

inline void ignore_snprintf(int rc) noexcept { (void)rc; }

#if defined(ENABLE_TELEMETRY)
void logProfilerToConsole(const Telemetry &t) noexcept;
#endif

} // namespace profiling
#endif
