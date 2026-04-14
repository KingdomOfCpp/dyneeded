#pragma once
#include <optional>
#include <span>

#include "../core/prelude.hpp"

namespace dyneeded
{
    struct Args
    {
        bool Recurse;
        bool Json;
        bool Tree;
        bool Text;
        bool Classic;
        bool Bible;
        bool HyprlandBtw;
        bool Version;
        bool Help;
        optional<string> Executable;

        static Args Parse(span<const string> args);
    };
} // dyneeded
