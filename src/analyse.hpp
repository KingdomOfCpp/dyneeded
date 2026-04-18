#pragma once
#include "../core/resolver.hpp"
#include "args.hpp"

namespace dyneeded
{
enum class OutputFormat
{
    Fancy,
    Text,
    Classic,
    Json,
    Tree,
};

void RunAnalysis(const Args &args, OutputFormat outputFormat);
} // namespace dyneeded