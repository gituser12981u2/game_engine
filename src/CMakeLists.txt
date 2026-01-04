add_subdirectory(backend)
add_subdirectory(render)
add_subdirectory(engine)
add_subdirectory(platform)

add_executable(quark main.cpp)

target_link_libraries(quark
  PRIVATE 
    quark::backend
    quark::render
    quark::engine
    quark::platform
)

