#pragma once
#include "dynamic_library.hpp"
#include "prelude.hpp"

namespace dyneeded {
    result<vector<DynamicLibrary> > ResolveDependencies(const fs::path &path);
}
