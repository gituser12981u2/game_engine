add_subdirectory(imgui)

add_library(quark_backend_ui STATIC 
  vk_ui_overlay.cpp
)

target_include_directories(quark_backend_ui
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_ui
  PRIVATE 
    quark::backend::core
    quark::platform::window
    quark::engine::logging
)

add_library(quark::backend::ui ALIAS quark_backend_ui)
