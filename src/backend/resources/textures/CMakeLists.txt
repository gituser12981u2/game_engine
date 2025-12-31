add_library(quark_backend_resources_textures STATIC 
    vk_texture_utils.cpp
)

target_include_directories(quark_backend_resources_textures
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_resources_textures
    PUBLIC
        Vulkan::Vulkan
        quark::backend::resources::images
        
)

add_library(quark::backend::resources::textures ALIAS quark_backend_resources_textures)
