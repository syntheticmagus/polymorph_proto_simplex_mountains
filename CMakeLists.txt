cmake_minimum_required(VERSION 3.10)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(simplex_mountains CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SUBMODULES_DIR "${PROJECT_SOURCE_DIR}/submodules")
set(PIPELINE_H_INCLUDE_DIR "${SUBMODULES_DIR}/pipeline_h")
set(CUTE_HEADERS_INCLUDE_DIR "${SUBMODULES_DIR}/cute_headers")

add_subdirectory("morphs/morph_cute_png" EXCLUDE_FROM_ALL)

set(SOURCES "main.cpp")

add_executable(simplex_mountains ${SOURCES})
target_link_libraries(simplex_mountains morph_cute_png)