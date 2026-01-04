add_library(quark_render_resources STATIC 
    resource_store.cpp
    mesh_store.cpp
    material_system.cpp
)

target_include_directories(quark_render_resources
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_render_resources
    PUBLIC
        quark::backend::core
        quark::backend::graphics
        quark::backend::frame
        quark::backend::profiling

        quark::backend::gpu::upload
        quark::backend::gpu::descriptors
        quark::backend::gpu::buffers 
        quark::backend::gpu::textures

        quark::engine::assets::stb_image
    PRIVATE
        glm::glm
)

add_library(quark::render::resources ALIAS quark_render_resources)
