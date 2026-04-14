#pragma once
#include "args.hpp"
#include "../core/dynamic_library.hpp"
#include "../core/resolver.hpp"

namespace dyneeded
{
    int RunTuiMode(const Args& args);
    void PrintResultsTui(const Args& args, const AnalysisResult& result);
}
