add_library(quark_backend_profiling_logging STATIC 
    console_sink.cpp
)

target_include_directories(quark_backend_profiling_logging
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_profiling_logging
    PUBLIC
        quark::backend::profiling::profilers
)

add_library(quark::backend::profiling::logging ALIAS quark_backend_profiling_logging)
