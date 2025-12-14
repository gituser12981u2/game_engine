add_library(engine_shader STATIC
  vulkan_shader.cpp
)

target_include_directories(engine_shader
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_link_libraries(engine_shader
  PUBLIC
    Vulkan::Vulkan
)

add_library(engine::shader ALIAS engine_shader)
