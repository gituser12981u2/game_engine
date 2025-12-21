add_subdirectory(core)
add_subdirectory(frame)
add_subdirectory(presentation)
add_subdirectory(render)
add_subdirectory(resources)
add_subdirectory(shaders)

add_library(engine_backend INTERFACE)
target_link_libraries(engine_backend INTERFACE
  engine::core
  engine::frame
  engine::presentation
  engine::render
  engine::resources
  engine::shaders
)

add_library(engine::backend ALIAS engine_backend)
