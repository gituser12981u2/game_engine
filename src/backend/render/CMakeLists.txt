add_subdirectory(resources)

add_library(quark_backend_render STATIC 
    renderer.cpp
    main_pass.cpp
    swapchain_attachments.cpp
    framebuffer_cache.cpp
    per_frame_data.cpp
)

target_include_directories(quark_backend_render
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_render
    PUBLIC
        quark::backend::frame
        quark::backend::presentation
        quark::backend::graphics
        quark::backend::core
        quark::backend::resources::descriptors
        quark::backend::profiling
        quark::backend::render::resources
        quark::engine::camera
        Vulkan::Vulkan
        glm::glm
    PRIVATE
        quark::engine::geometry
)

add_library(quark::backend::render ALIAS quark_backend_render)
