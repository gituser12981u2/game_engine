add_library(quark_engine_logging STATIC 
   log.cpp 
)

target_include_directories(quark_engine_logging
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(quark_engine_logging
    PUBLIC
        spdlog::spdlog
)

add_library(quark::engine::logging ALIAS quark_engine_logging)
