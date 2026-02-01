add_subdirectory(app)
add_subdirectory(camera)
add_subdirectory(geometry)
add_subdirectory(assets)
add_subdirectory(jobs)
add_subdirectory(editor)
add_subdirectory(logging)

add_library(quark_engine INTERFACE)
target_link_libraries(quark_engine INTERFACE
  quark::engine::app
  quark::engine::assets
  quark::engine::camera
  quark::engine::geometry
  quark::engine::jobs
  quark::engine::editor
  quark::engine::logging
)

add_library(quark::engine ALIAS quark_engine)
