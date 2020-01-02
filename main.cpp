#include "morph_opensimplex.h"
#include "morph_cute_png.h"

#include <algorithm>
#include <cassert>

namespace
{
    void InPlaceTransformValue(std::vector<double>& values, double scalar)
    {
        double maxValue = std::numeric_limits<double>::min();
        double minValue = std::numeric_limits<double>::max();
        for (double& value : values)
        {
            value = std::abs(value);
            maxValue = std::max(maxValue, value);
            minValue = std::min(minValue, value);
        }

        double normalizer = 1.0 / (maxValue - minValue);
        for (double& value : values)
        {
            value = scalar * (1.0 - normalizer * (value - minValue));
        }
    }

    void InPlaceAdd(std::vector<double>& values, const std::vector<double>& addends)
    {
        assert(values.size() <= addends.size());
        for (size_t idx = 0; idx < values.size(); ++idx)
        {
            values[idx] += addends[idx];
        }
    }
}

PIPELINE_TYPE(SummedOctaves, std::vector<double>);
PIPELINE_TYPE(MaxOctaveValue, double);

namespace sx = morph_opensimplex;
namespace cp = morph_cute_png;

PIPELINE_CONTEXT(Initialize,
    IN_CONTRACT(),
    OUT_CONTRACT(cp::FileName, sx::Width, sx::Height, SummedOctaves, MaxOctaveValue));

PIPELINE_CONTEXT(PrepOpenSimplexMap,
    IN_CONTRACT(),
    OUT_CONTRACT(sx::Frequency));

PIPELINE_CONTEXT(TransformValues,
    IN_CONTRACT(sx::Values, SummedOctaves, MaxOctaveValue),
    OUT_CONTRACT(sx::Values, SummedOctaves, MaxOctaveValue));
template<size_t OctaveScale>
void Run(TransformValues& context)
{
    auto& values = context.ModifyValues();
    InPlaceTransformValue(values, OctaveScale);
    InPlaceAdd(context.ModifySummedOctaves(), values);
    context.SetMaxOctaveValue(context.GetMaxOctaveValue() + OctaveScale);
}

PIPELINE_CONTEXT(ConvertSimplexMapToPng,
    IN_CONTRACT(sx::Height, sx::Width, SummedOctaves, MaxOctaveValue),
    OUT_CONTRACT(cp::PixelsWidth, cp::PixelsHeight, cp::PixelsData));

// TODO: Atrocious nonsense like this is EXACTLY why we need to support
// proper meta-morphs in the pipeline.
#define ADD_OCTAVE(frequency, scale)                                    \
->Then<PrepOpenSimplexMap>([](PrepOpenSimplexMap& context)              \
{                                                                       \
    context.SetFrequency(frequency);                                    \
})->Then<GenerateOpenSimplexMap>([](GenerateOpenSimplexMap& context)    \
{                                                                       \
    Run(context);                                                       \
})->Then<TransformValues>([](TransformValues& context)                  \
{                                                                       \
    Run<scale>(context);                                                \
})

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

        std::vector<double> summedOctaves{};
        summedOctaves.resize(args.Width * args.Height);
        context.SetSummedOctaves(summedOctaves);
        context.SetMaxOctaveValue(0);
    })
    ADD_OCTAVE(0.005, 32)
    ADD_OCTAVE(0.01, 16)
    ADD_OCTAVE(0.02, 8)
    ADD_OCTAVE(0.04, 4)
    ADD_OCTAVE(0.08, 2)
    ADD_OCTAVE(0.16, 1)
    ->Then<ConvertSimplexMapToPng>([](ConvertSimplexMapToPng& context)
    {
        const auto& values = context.GetSummedOctaves();
        const auto normalizingScalar = 1.0 / context.GetMaxOctaveValue();

        // Convert values to pixels.
        std::vector<cp::Pixel> pixels{};
        pixels.reserve(values.size());
        std::transform(values.begin(), values.end(), std::back_inserter(pixels), [normalizingScalar](double value)
        {
            constexpr double MAXVAL = std::numeric_limits<uint8_t>::max();
            uint8_t byteVal = static_cast<uint8_t>(std::clamp((value * normalizingScalar) * MAXVAL, 0.0, MAXVAL));
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