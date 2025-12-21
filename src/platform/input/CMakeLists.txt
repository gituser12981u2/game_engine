add_library(engine_input STATIC 
    camera_controller.cpp
)

target_include_directories(engine_input
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_input
    PUBLIC
        Vulkan::Vulkan
        glm::glm
        glfw
)

add_library(engine::input ALIAS engine_input)
