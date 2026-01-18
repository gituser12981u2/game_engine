add_library(quark_backend_profiling_telemetry STATIC 
    telemetry.cpp
    publish.cpp
)

target_include_directories(quark_backend_profiling_telemetry
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_profiling_telemetry
    PUBLIC
        quark::backend::profiling::profilers
)

add_library(quark::backend::profiling::telemetry ALIAS quark_backend_profiling_telemetry)
