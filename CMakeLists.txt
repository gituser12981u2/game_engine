cmake_minimum_required(VERSION 3.15)
project(GameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 CONFIG REQUIRED)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

find_package(glm CONFIG REQUIRED)

find_package(Stb REQUIRED)



find_path(CGLTF_INCLUDE_DIRS "cgltf.h")

add_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-Wall>
  $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
  $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
  $<$<COMPILE_LANGUAGE:CXX>:-pedantic-errors>
  $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
)



add_subdirectory(src)

