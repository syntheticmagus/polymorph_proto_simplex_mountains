#include "morph_opensimplex.h"
#include "morph_cute_png.h"

#include <algorithm>

namespace sx = morph_opensimplex;
namespace cp = morph_cute_png;

PIPELINE_CONTEXT(Initialize,
    IN_CONTRACT(),
    OUT_CONTRACT(cp::FileName, sx::Width, sx::Height, sx::Frequency));

PIPELINE_CONTEXT(ConvertSimplexMapToPng,
    IN_CONTRACT(sx::Height, sx::Width, sx::Values),
    OUT_CONTRACT(cp::PixelsWidth, cp::PixelsHeight, cp::PixelsData));

int main()
{
    struct
    {
        const char* FileName{ "C:\\scratch\\cp_output.png" };
        const size_t Width{ 1024 };
        const size_t Height{ 1024 };
        const double Frequency{ 0.01 };
    } args;

    auto pipeline = Pipeline::First<Initialize>([&args](Initialize& context)
    {
        context.SetFileName(args.FileName);
        context.SetWidth(args.Width);
        context.SetHeight(args.Height);
        context.SetFrequency(args.Frequency);
    })->Then<GenerateOpenSimplexMap>([](GenerateOpenSimplexMap& context)
    {
        Run(context);
    })->Then<ConvertSimplexMapToPng>([](ConvertSimplexMapToPng& context)
    {
        auto& values = context.GetValues();

        std::vector<cp::Pixel> pixels{};
        pixels.reserve(values.size());
        std::transform(values.begin(), values.end(), std::back_inserter(pixels), [](double value)
        {
            constexpr double MAXVAL = std::numeric_limits<uint8_t>::max();
            uint8_t byteVal = static_cast<uint8_t>(std::clamp(((value + 1.0) / 2.0) * MAXVAL, 0.0, MAXVAL));
            return cp::Pixel
            {
                byteVal,
                byteVal,
                byteVal,
                std::numeric_limits<uint8_t>::max()
            };
        });

        context.SetPixelsWidth(context.GetWidth());
        context.SetPixelsHeight(context.GetHeight());
        context.SetPixelsData(pixels);
    })->Then<ExportPng>([](ExportPng& context)
    {
        Run(context);
    });
    pipeline->Run();

    return 0;
}