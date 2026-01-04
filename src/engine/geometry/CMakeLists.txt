add_library(quark_engine_geometry STATIC 
   primitives.cpp 
)

target_include_directories(quark_engine_geometry
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_engine_geometry
    PUBLIC
        glm::glm

        quark::render

)

add_library(quark::engine::geometry ALIAS quark_engine_geometry)
