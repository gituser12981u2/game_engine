add_library(quark_backend_gpu_descriptors STATIC 
    vk_material_sets.cpp
    vk_scene_sets.cpp
    vk_shader_interface.cpp
)

target_include_directories(quark_backend_gpu_descriptors
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_gpu_descriptors
    PUBLIC
        Vulkan::Vulkan
        glm::glm

        quark::backend::gpu::buffers
        quark::backend::gpu::textures
    PRIVATE
        quark::render::scene
)

add_library(quark::backend::gpu::descriptors ALIAS quark_backend_gpu_descriptors)
