add_library(quark_backend_frame STATIC 
    vk_commands.cpp
    vk_frame_manager.cpp
)

target_include_directories(quark_backend_frame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(quark_backend_frame
    PUBLIC
      Vulkan::Vulkan
      quark::backend::core
      quark::backend::profiling
)

add_library(quark::backend::frame ALIAS quark_backend_frame)
