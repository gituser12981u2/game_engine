add_library(engine_shaders STATIC
  vk_shader.cpp
)

target_include_directories(engine_shaders
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_link_libraries(engine_shaders
  PUBLIC
    Vulkan::Vulkan
)

add_library(engine::shaders ALIAS engine_shaders)
