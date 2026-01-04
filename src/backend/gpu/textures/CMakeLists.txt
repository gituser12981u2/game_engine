add_library(quark_backend_gpu_textures STATIC 
    vk_texture_utils.cpp
)

target_include_directories(quark_backend_gpu_textures
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_gpu_textures
    PUBLIC
        Vulkan::Vulkan

        quark::backend::gpu::images
)

add_library(quark::backend::gpu::textures ALIAS quark_backend_gpu_textures)
