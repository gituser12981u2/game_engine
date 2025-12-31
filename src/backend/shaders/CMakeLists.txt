

add_library(quark_backend_shaders STATIC
  vk_shader.cpp
)

target_include_directories(quark_backend_shaders
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(quark_backend_shaders
  PUBLIC
    Vulkan::Vulkan
)

add_library(quark::backend::shaders ALIAS quark_backend_shaders)
