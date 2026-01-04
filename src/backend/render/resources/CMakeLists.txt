add_library(quark_backend_render_resources STATIC 
    resource_store.cpp
    mesh_store.cpp
    material_system.cpp
)

target_include_directories(quark_backend_render_resources
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_render_resources
    PUBLIC
        quark::backend::graphics
        quark::backend::frame
        quark::backend::core
        quark::backend::profiling
        quark::backend::resources::upload
        quark::backend::resources::descriptors
        quark::backend::resources::buffers 
        quark::backend::resources::textures
        quark::engine::assets::stb_image
    PRIVATE
        glm::glm
)

add_library(quark::backend::render::resources ALIAS quark_backend_render_resources)
