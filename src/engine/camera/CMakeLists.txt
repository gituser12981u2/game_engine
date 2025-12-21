add_library(engine_camera STATIC 
    camera.cpp
)

target_include_directories(engine_camera
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_camera
    PUBLIC
        Vulkan::Vulkan
        glm::glm
)

add_library(engine::camera ALIAS engine_camera)
