set(SOURCES
    "include/morph_opensimplex.h"
    "source/morph_opensimplex.cpp"
    "source/OpenSimplexNoise.hpp")

add_library(morph_opensimplex ${SOURCES})
set_target_properties(morph_opensimplex PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(morph_opensimplex PRIVATE ${PIPELINE_H_INCLUDE_DIR})

target_include_directories(morph_opensimplex PUBLIC "include")
