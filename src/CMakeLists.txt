add_subdirectory(backend)

add_executable(engine_app main.cpp)

target_link_libraries(engine_app
  PRIVATE 
    glfw
    engine::backend
)

