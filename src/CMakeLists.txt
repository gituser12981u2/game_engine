add_subdirectory(backend)
add_subdirectory(engine)
add_subdirectory(platform)

add_executable(engine_app main.cpp)

target_link_libraries(engine_app
  PRIVATE 
    glfw
    engine::backend
    engine::engine
    engine::platform
)

