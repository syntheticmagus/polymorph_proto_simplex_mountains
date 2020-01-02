#include "morph_cute_png.h"

#include <iostream>

namespace
{
    using Pixel = std::array<uint8_t, 4>;

    PIPELINE_TYPE(FileName, const char*);
    PIPELINE_TYPE(PixelsWidth, size_t);
    PIPELINE_TYPE(PixelsHeight, size_t);
    PIPELINE_TYPE(PixelsData, std::vector<Pixel>);
}

PIPELINE_CONTEXT(GenerateImage,
    IN_CONTRACT(),
    OUT_CONTRACT(PixelsWidth, PixelsHeight, PixelsData));
template<size_t Width, size_t Height>
void Run(GenerateImage& context)
{
    std::vector<Pixel> pixels{};
    pixels.resize(Width * Height);
    for (size_t y = 0; y < Height; ++y)
    {
        for (size_t x = 0; x < Width; ++x)
        {
            auto& pixel = pixels[x + y * Width];
            pixel[0] = 255;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = ((x % 2) != (y % 2) ) ? 255 : 0;
        }
    }

    context.SetPixelsWidth(Width);
    context.SetPixelsHeight(Height);
    context.SetPixelsData(pixels);
}

PIPELINE_CONTEXT(FileNameSetter,
    IN_CONTRACT(),
    OUT_CONTRACT(FileName));

int main()
{
    std::cout << "Hello, world!" << std::endl;

    auto pipeline = Pipeline::First<GenerateImage>([](GenerateImage& context)
    {
        Run<256, 256>(context);
    })->Then<FileNameSetter>([](FileNameSetter& context)
    {
        context.SetFileName("C:\\scratch\\cp_output.png");
    })->Then<ExportPng>([](ExportPng& context)
    {
        Run(context);
    });
    pipeline->Run(pipeline->CreateManyMap());

    return 0;
}