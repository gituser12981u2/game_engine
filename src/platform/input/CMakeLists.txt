add_library(quark_platform_input STATIC 
    camera_controller.cpp
)

target_include_directories(quark_platform_input
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_platform_input
    PUBLIC
        Vulkan::Vulkan
        glm::glm
        glfw
        quark::platform::window
        quark::engine::camera
)

add_library(quark::platform::input ALIAS quark_platform_input)
