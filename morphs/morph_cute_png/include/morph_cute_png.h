#pragma once

#include <pipeline.h>

#include <array>
#include <vector>

namespace morph_cute_png
{
    using Pixel = std::array<uint8_t, 4>;

    PIPELINE_TYPE(FileName, const char*);
    PIPELINE_TYPE(PixelsWidth, size_t);
    PIPELINE_TYPE(PixelsHeight, size_t);
    PIPELINE_TYPE(PixelsData, std::vector<Pixel>);

    using ExportInContract = IN_CONTRACT(FileName, PixelsWidth, PixelsHeight, PixelsData);
}

PIPELINE_CONTEXT(ExportPng,
    morph_cute_png::ExportInContract,
    OUT_CONTRACT());
void Run(ExportPng& context);