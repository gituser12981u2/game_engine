add_library(quark_render_scene STATIC 
    scene_data.cpp
)

target_include_directories(quark_render_scene
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_render_scene
    PUBLIC
        Vulkan::Vulkan
        glm::glm

        quark::backend::core

        quark::backend::gpu::buffers
        quark::backend::gpu::descriptors
        quark::backend::gpu::upload
        quark::backend::profiling

        quark::engine::camera
    PRIVATE
        quark::render::resources
)

add_library(quark::render::scene ALIAS quark_render_scene)
