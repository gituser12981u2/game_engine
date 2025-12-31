add_library(quark_engine_assets_stb_image STATIC 
    stb_image_impl.cpp
    stb_image_loader.cpp
)

target_include_directories(quark_engine_assets_stb_image
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src
    PRIVATE
      ${Stb_INCLUDE_DIR}
)

target_link_libraries(quark_engine_assets_gltf
    PUBLIC 
      glm::glm
      quark::backend::render
)

add_library(quark::engine::assets::stb_image ALIAS quark_engine_assets_stb_image)
