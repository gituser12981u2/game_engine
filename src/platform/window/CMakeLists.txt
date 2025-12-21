add_library(engine_window STATIC 
    glfw_window.cpp
)

target_include_directories(engine_window
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_window
    PUBLIC
        glfw
)

add_library(engine::window ALIAS engine_window)
