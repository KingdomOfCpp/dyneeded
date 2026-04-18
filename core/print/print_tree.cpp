#include "print_tree.hpp"
#include <fmt/ranges.h>

namespace dyneeded
{
void PrintElfTree(const ElfExecutable &exe, string prefix, bool isLast)
{
    // ├── or └── for current node
    fmt::print("{}{}", prefix, isLast ? "└── " : "├── ");

    // print name + path + versions
    fmt::print("{}", exe.GetName());
    if (auto *p = exe.GetPath())
        fmt::print(" ({})", p->string());
    if (auto v = exe.GetVersion())
        fmt::print(" [{}]", *v);
    if (auto &nv = exe.GetNeededVersions(); !nv.empty())
        fmt::print(" needs: {}", fmt::join(nv, ", "));
    fmt::println("");

    // recurse with extended prefix
    auto deps = exe.GetDependencies();
    for (size_t i = 0; i < deps.size(); i++)
    {
        bool last = (i == deps.size() - 1);
        // if current node is not last, children need a │ continuation
        PrintElfTree(deps[i], prefix + (isLast ? "    " : "│   "), last);
    }
}
}