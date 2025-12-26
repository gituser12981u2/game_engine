add_library(engine_window STATIC 
    glfw_window.cpp
)

target_include_directories(engine_window
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(engine_window
    PUBLIC
      glfw
      Vulkan::Vulkan
)

add_library(engine::window ALIAS engine_window)
