add_library(engine_presentation STATIC 
    vk_presenter.cpp
    vk_swapchain.cpp
)

target_include_directories(engine_presentation
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(engine_presentation
    PUBLIC
        Vulkan::Vulkan
)

add_library(engine::presentation ALIAS engine_presentation)
