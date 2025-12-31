add_library(quark_backend_resources_descriptors STATIC 
    vk_material_sets.cpp
    vk_per_frame_sets.cpp
    vk_shader_interface.cpp
)

target_include_directories(quark_backend_resources_descriptors
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_resources_descriptors
    PUBLIC
        Vulkan::Vulkan
        glm::glm
        quark::backend::resources::buffers
        quark::backend::resources::textures
)

add_library(quark::backend::resources::descriptors ALIAS quark_backend_resources_descriptors)
