add_library(quark_backend_gpu_images STATIC 
    vk_image.cpp
    vk_depth_image.cpp
)

target_include_directories(quark_backend_gpu_images
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_gpu_images
    PUBLIC
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
)

add_library(quark::backend::gpu::images ALIAS quark_backend_gpu_images)
