add_library(quark_backend_resources_upload STATIC
    vk_buffer_uploader.cpp
    vk_texture_uploader.cpp
    vk_instance_uploader.cpp
    vk_material_uploader.cpp
    vk_upload_context.cpp
)

target_include_directories(quark_backend_resources_upload
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_resources_upload
    PUBLIC
      Vulkan::Vulkan
      GPUOpen::VulkanMemoryAllocator
      quark::backend::frame
      quark::backend::profiling
      quark::backend::resources::buffers
      quark::backend::resources::textures
      quark::backend::core
      glm::glm
)

add_library(quark::backend::resources::upload ALIAS quark_backend_resources_upload)
