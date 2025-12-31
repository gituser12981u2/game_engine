add_library(quark_engine_assets_gltf STATIC 
    cgltf_impl.cpp
    gltf_asset.cpp
    gltf_cpu_loader.cpp
    gltf_gpu_builder.cpp
)

target_include_directories(quark_engine_assets_gltf
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src
    PRIVATE
      ${CGLTF_INCLUDE_DIRS}
)

target_link_libraries(quark_engine_assets_gltf
    PUBLIC 
      glm::glm
      quark::backend::render
)

add_library(quark::engine::assets::gltf ALIAS quark_engine_assets_gltf)
