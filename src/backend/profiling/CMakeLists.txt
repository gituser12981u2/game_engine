add_subdirectory(logging)
add_subdirectory(profilers)
add_subdirectory(telemetry)

add_library(quark_backend_profiling INTERFACE)
target_link_libraries(quark_backend_profiling INTERFACE
    quark::backend::profiling::logging
    quark::backend::profiling::profilers
    quark::backend::profiling::telemetry
)

add_library(quark::backend::profiling ALIAS quark_backend_profiling)
