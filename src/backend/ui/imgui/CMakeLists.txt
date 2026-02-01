add_library(quark_backend_ui_imgui STATIC 
  imgui_overlay_sink.cpp
)

target_include_directories(quark_backend_ui_imgui
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src     
)

target_link_libraries(quark_backend_ui_imgui
  PRIVATE 
    quark::backend::core
    quark::platform::window
    quark::engine::logging
    imgui::imgui
)

add_library(quark::backend::ui::imgui ALIAS quark_backend_ui_imgui)
