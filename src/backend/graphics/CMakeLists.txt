add_library(quark_backend_graphics STATIC 
    vk_pipeline.cpp
    vk_render_pass.cpp
    vk_framebuffers.cpp
)

target_include_directories(quark_backend_graphics
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_graphics
    PUBLIC
      Vulkan::Vulkan
    PRIVATE
      glm::glm
      quark::backend::shaders
)

add_library(quark::backend::graphics ALIAS quark_backend_graphics)
