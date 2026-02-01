add_library(quark_engine_editor STATIC 
  editor.cpp
)

target_include_directories(quark_engine_editor
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_engine_editor
  PRIVATE
    imgui::imgui
)

add_library(quark::engine::editor ALIAS quark_engine_editor)
