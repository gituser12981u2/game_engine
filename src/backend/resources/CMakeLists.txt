add_library(engine_resources STATIC 
    vk_buffer.cpp
    vk_uploader.cpp
    vk_per_frame_uniform.cpp
    vk_depth_image.cpp
)

target_include_directories(engine_resources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_resources
    PUBLIC
        Vulkan::Vulkan
    PRIVATE
        glm::glm
)

add_library(engine::resources ALIAS engine_resources)
