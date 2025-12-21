add_library(engine_geometry STATIC 
   primitives.cpp 
)

target_include_directories(engine_geometry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_geometry
    PUBLIC
        glm::glm
)

add_library(engine::geometry ALIAS engine_geometry)
