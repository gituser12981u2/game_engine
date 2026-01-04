add_library(quark_render_upload STATIC 
    upload_manager.cpp
)

target_include_directories(quark_render_upload
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_render_upload
    PUBLIC
        Vulkan::Vulkan

        quark::backend::core
        quark::backend::profiling

        quark::backend::gpu::upload
)

add_library(quark::render::upload ALIAS quark_render_upload)
