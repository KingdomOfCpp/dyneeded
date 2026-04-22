#pragma once
#include "core/prelude.hpp"

namespace dyneeded
{
    struct VersionInfo
    {
        string Prefix;
        vector<int> Parts;

        static result<VersionInfo> Parse(string_view str);
    };
} // namespace dyneeded
