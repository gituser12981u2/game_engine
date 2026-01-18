add_library(quark_backend_profiling_profilers STATIC 
    cpu_profiler.cpp
    upload_profiler.cpp
    vk_gpu_profiler.cpp
)

target_include_directories(quark_backend_profiling_profilers
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_profiling_profilers
    PUBLIC
        Vulkan::Vulkan
        quark::backend::core
)

add_library(quark::backend::profiling::profilers ALIAS quark_backend_profiling_profilers)
