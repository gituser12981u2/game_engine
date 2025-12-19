add_library(engine_resources STATIC 
    vk_buffer.cpp
    vk_uploader.cpp
)

target_include_directories(engine_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_resources
    PUBLIC
        Vulkan::Vulkan
)

add_library(engine::resources ALIAS engine_resources)
