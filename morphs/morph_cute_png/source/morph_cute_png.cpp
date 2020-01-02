#include "morph_cute_png.h"

#define CUTE_PNG_IMPLEMENTATION
#include <cute_png.h>

#include <iostream>


void Run(ExportPng& context)
{
    auto width = context.GetPixelsWidth();
    auto height = context.GetPixelsHeight();
    
    auto image = cp_load_blank(static_cast<int>(width), static_cast<int>(height));
    std::memcpy(image.pix, context.GetPixelsData().data(), width * height * sizeof(cp_pixel_t));
    
    cp_save_png(context.GetFileName(), &image);
}