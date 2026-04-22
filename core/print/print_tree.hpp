#pragma once
#include "core/formats/elf_executable.hpp"
#include <fmt/ranges.h>

namespace dyneeded
{
    template <typename TExecutable>
    void PrintTree(const TExecutable& exe, string prefix = "", bool isLast = true)
    {
        // ├── or └── for current node
        fmt::print("{}{}", prefix, isLast ? "└── " : "├── ");

        // print name + path
        fmt::print("{}", exe.GetName());
        if (auto* p = exe.GetPath())
            fmt::print(" ({})", p->string());
        fmt::println("");

        // recurse with extended prefix
        auto deps = exe.GetDirectDependencies();
        for (size_t i = 0; i < deps.size(); i++)
        {
            bool last = (i == deps.size() - 1);
            // if current node is not last, children need a │ continuation
            PrintTree(deps[i], prefix + (isLast ? "    " : "│   "), last);
        }
    }
}
