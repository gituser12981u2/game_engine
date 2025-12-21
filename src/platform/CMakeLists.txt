add_subdirectory(input)
add_subdirectory(window)

add_library(engine_platform INTERFACE)
target_link_libraries(engine_platform INTERFACE
  engine::input
  engine::window
)

add_library(engine::platform ALIAS engine_platform)
