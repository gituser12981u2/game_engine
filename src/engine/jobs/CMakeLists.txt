add_library(quark_engine_jobs STATIC 
   job_system.cpp 
)

target_include_directories(quark_engine_jobs
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

add_library(quark::engine::jobs ALIAS quark_engine_jobs)
