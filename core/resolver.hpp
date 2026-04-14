#pragma once
#include "dynamic_library.hpp"
#include "prelude.hpp"

namespace dyneeded {
    struct AnalysisResult {
        vector<DynamicLibrary> Dependencies;
#ifdef DYNEEDED_LINUX
        result<pair<int, int>> MinimalGlibc;
        result<pair<int, int>> MinimalGlibcxx;
#elifdef DYNEEDED_WINDOWS
        optional<int> MinimalMsvcRuntime;
#endif

        static result<AnalysisResult> AnalyzeExecutable(const fs::path& executable, bool recurse);
    };
}
