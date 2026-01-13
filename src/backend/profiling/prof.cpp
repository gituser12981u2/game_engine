#include "backend/profiling/prof.hpp"

namespace profiling {

thread_local Telemetry *g_tls = nullptr;

Telemetry *tlsTelemetry() noexcept { return g_tls; }
void setTlsTelemetry(Telemetry *t) noexcept { g_tls = t; }

} // namespace profiling
