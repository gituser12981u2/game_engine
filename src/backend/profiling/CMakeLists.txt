add_library(quark_backend_profiling STATIC 
    vk_gpu_profiler.cpp
    profiling_logger.cpp
)

target_include_directories(quark_backend_profiling
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_profiling
    PUBLIC
        Vulkan::Vulkan
        quark::backend::core
)

add_library(quark::backend::profiling ALIAS quark_backend_profiling)
