add_subdirectory(buffers)
add_subdirectory(descriptors)
add_subdirectory(images)
add_subdirectory(upload)
add_subdirectory(textures)

add_library(quark_backend_resources INTERFACE)
target_link_libraries(quark_backend_resources INTERFACE
  quark::backend::resources::buffers
  quark::backend::resources::descriptors
  quark::backend::resources::images
  quark::backend::resources::textures
  quark::backend::resources::upload
)

add_library(quark::backend::resources ALIAS quark_backend_resources)
