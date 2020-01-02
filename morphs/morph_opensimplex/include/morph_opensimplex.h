#pragma once

#include <pipeline.h>

#include <vector>

namespace morph_opensimplex
{
    PIPELINE_TYPE(Width, size_t);
    PIPELINE_TYPE(Height, size_t);
    PIPELINE_TYPE(Frequency, double);
    PIPELINE_TYPE(Values, std::vector<double>);

    using InContract = IN_CONTRACT(Width, Height, Frequency);
    using OutContract = OUT_CONTRACT(Values);
}

PIPELINE_CONTEXT(GenerateOpenSimplexMap, 
    morph_opensimplex::InContract, 
    morph_opensimplex::OutContract);
void Run(GenerateOpenSimplexMap&);