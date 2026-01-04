add_library(quark_backend_gpu_upload STATIC
    vk_buffer_uploader.cpp
    vk_texture_uploader.cpp
    vk_instance_uploader.cpp
    vk_material_uploader.cpp
    vk_upload_context.cpp
)

target_include_directories(quark_backend_gpu_upload
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_gpu_upload
    PUBLIC
      Vulkan::Vulkan
      GPUOpen::VulkanMemoryAllocator
      glm::glm

      quark::backend::core
      quark::backend::frame
      quark::backend::profiling
      quark::backend::gpu::buffers
      quark::backend::gpu::textures
)

add_library(quark::backend::gpu::upload ALIAS quark_backend_gpu_upload)
