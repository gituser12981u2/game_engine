add_library(engine_frame STATIC 
    vk_commands.cpp
    vk_frame_manager.cpp
)

target_include_directories(engine_frame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_frame
    PUBLIC
        Vulkan::Vulkan
)

add_library(engine::frame ALIAS engine_frame)
