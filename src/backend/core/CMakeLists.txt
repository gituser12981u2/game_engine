add_library(engine_core STATIC 
    vk_instance.cpp
    vk_device.cpp
)

target_include_directories(engine_core 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_core
    PUBLIC
        Vulkan::Vulkan
)

add_library(engine::core ALIAS engine_core)
