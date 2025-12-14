add_library(engine_presentation STATIC 
    vk_presenter.cpp
    vk_swapchain.cpp
)

target_include_directories(engine_presentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_presentation
    PUBLIC
        Vulkan::Vulkan
    PRIVATE
        glfw
)

add_library(engine::presentation ALIAS engine_presentation)
