#include "morph_opensimplex.h"

#include "OpenSimplexNoise.hpp"

void Run(GenerateOpenSimplexMap& context)
{

    size_t width = context.GetWidth();
    size_t height = context.GetHeight();
    double frequency = context.GetFrequency();

    std::vector<double> values{};
    values.resize(width * height);

    OpenSimplexNoise noise{};
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            size_t idx = x + y * width;
            values[idx] = noise.Evaluate(x * frequency, y * frequency);
        }
    }

    context.SetValues(values);
}