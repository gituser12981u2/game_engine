add_library(engine_render STATIC 
    renderer.cpp
    vk_framebuffers.cpp
    vk_pipeline.cpp
    vk_render_pass.cpp
)

target_include_directories(engine_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_render
    PUBLIC
        engine::frame
        engine::presentation
        Vulkan::Vulkan
        glm::glm
        glfw
)

add_library(engine::render ALIAS engine_render)
