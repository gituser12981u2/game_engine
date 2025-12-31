add_library(quark_backend_presentation STATIC 
    vk_presenter.cpp
    vk_swapchain.cpp
)

target_include_directories(quark_backend_presentation
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_presentation
    PUBLIC
        Vulkan::Vulkan
        quark::backend::core
        quark::platform::window
)

add_library(quark::backend::presentation ALIAS quark_backend_presentation)
