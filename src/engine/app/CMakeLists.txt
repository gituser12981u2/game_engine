add_library(quark_engine_app STATIC 
  app.cpp
)

target_include_directories(quark_engine_app
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_engine_app
    PUBLIC
      quark::backend::core
      quark::backend::presentation
      quark::render
      quark::engine::geometry
      quark::platform::window
      quark::engine::jobs

    PRIVATE
      Vulkan::Vulkan
      quark::engine::editor
      quark::backend::ui::imgui
)

add_library(quark::engine::app ALIAS quark_engine_app)
