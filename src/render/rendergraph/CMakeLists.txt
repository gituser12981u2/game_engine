add_library(quark_render_rendergraph STATIC 
    main_pass.cpp
    swapchain_attachments.cpp
    framebuffer_cache.cpp
)

target_include_directories(quark_render_rendergraph
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_render_rendergraph
    PUBLIC
        Vulkan::Vulkan

        quark::backend::core
        quark::backend::frame
        quark::backend::presentation
        quark::backend::graphics

        quark::backend::gpu::descriptors
        quark::backend::gpu::images
)

add_library(quark::render::rendergraph ALIAS quark_render_rendergraph)


