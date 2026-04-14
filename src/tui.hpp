#pragma once
#include "args.hpp"
#include "../core/dynamic_library.hpp"

namespace dyneeded {
    int RunTuiMode(const Args& args);
    void PrintResultsTui(const Args& args, span<const DynamicLibrary> libraries);
}