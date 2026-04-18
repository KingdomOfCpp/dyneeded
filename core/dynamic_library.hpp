#pragma once
#include "prelude.hpp"
#include <filesystem>
#include <optional>

namespace dyneeded
{
struct DynamicLibrary
{
    string Name;
    fs::path Path;
    vector<string> Versions;
};
} // namespace dyneeded
