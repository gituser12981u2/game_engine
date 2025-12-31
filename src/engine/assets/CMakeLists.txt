add_subdirectory(gltf)
add_subdirectory(stb_image)

add_library(quark_engine_assets INTERFACE)
target_link_libraries(quark_engine_assets INTERFACE
  quark::engine::assets::gltf 
  quark::engine::assets::stb_image
)

add_library(quark::engine::assets ALIAS quark_engine_assets)
