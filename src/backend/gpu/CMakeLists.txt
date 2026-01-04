add_subdirectory(buffers)
add_subdirectory(descriptors)
add_subdirectory(images)
add_subdirectory(upload)
add_subdirectory(textures)

add_library(quark_backend_gpu INTERFACE)
target_link_libraries(quark_backend_gpu INTERFACE
  quark::backend::gpu::buffers
  quark::backend::gpu::descriptors
  quark::backend::gpu::images
  quark::backend::gpu::textures
  quark::backend::gpu::upload
)

add_library(quark::backend::gpu ALIAS quark_backend_gpu)
