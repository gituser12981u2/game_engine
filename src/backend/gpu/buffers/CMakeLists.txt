add_library(quark_backend_gpu_buffers STATIC 
    vk_buffer.cpp
    vk_per_frame_uniform_buffers.cpp
)

target_include_directories(quark_backend_gpu_buffers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_gpu_buffers
    PUBLIC
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
)

add_library(quark::backend::gpu::buffers ALIAS quark_backend_gpu_buffers)
