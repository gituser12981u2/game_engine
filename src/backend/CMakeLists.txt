add_subdirectory(core)
add_subdirectory(frame)
add_subdirectory(presentation)
add_subdirectory(graphics)
add_subdirectory(gpu)
add_subdirectory(shaders)
add_subdirectory(profiling)

add_library(quark_backend INTERFACE)
target_link_libraries(quark_backend INTERFACE
  quark::backend::core
  quark::backend::frame
  quark::backend::presentation
  quark::backend::graphics
  quark::backend::gpu
  quark::backend::shaders
  quark::backend::profiling
)

add_library(quark::backend ALIAS quark_backend)
