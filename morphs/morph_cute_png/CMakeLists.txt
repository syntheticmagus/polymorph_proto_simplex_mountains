set(SOURCES
    "include/morph_cute_png.h"
    "source/morph_cute_png.cpp")

add_library(morph_cute_png ${SOURCES})
set_target_properties(morph_cute_png PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(morph_cute_png PRIVATE ${PIPELINE_H_INCLUDE_DIR})
target_include_directories(morph_cute_png PRIVATE ${CUTE_HEADERS_INCLUDE_DIR})

target_include_directories(morph_cute_png PUBLIC "include")

if (MSVC)
    target_compile_definitions(morph_cute_png PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
