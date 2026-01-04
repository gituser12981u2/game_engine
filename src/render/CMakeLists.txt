add_subdirectory(rendergraph)
add_subdirectory(resources)
add_subdirectory(scene)
add_subdirectory(upload)

add_library(quark_render STATIC 
    renderer.cpp
)

target_include_directories(quark_render
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_render
    PUBLIC
        Vulkan::Vulkan
        glm::glm

        quark::backend::core
        quark::backend::frame
        quark::backend::presentation
        quark::backend::graphics
        quark::backend::profiling

        quark::render::rendergraph
        quark::render::resources
        quark::render::scene
        quark::render::upload

        quark::backend::gpu::descriptors
        quark::backend::gpu::upload

        quark::engine::camera
    PRIVATE
        quark::engine::geometry
)

add_library(quark::render ALIAS quark_render)
