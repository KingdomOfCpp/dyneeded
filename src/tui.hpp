#pragma once
#include "../core/dynamic_library.hpp"
#include "../core/resolver.hpp"
#include "args.hpp"

namespace dyneeded
{
int RunTuiMode(const Args &args);
void PrintResultsTui(const Args &args, const AnalysisResult &result);
} // namespace dyneeded
