#include "args.hpp"
#include "fmt/base.h"
#include <fmt/core.h>

namespace dyneeded
{
    Args Args::Parse(span<const string> args)
    {
        auto result = Args();
        for (const auto& arg : args)
        {
            if (arg == "-h" || arg == "--help")
            {
                result.Help = true;
            }
            else if (arg == "-v" || arg == "--version")
            {
                result.Version = true;
            }
            else if (arg == "-b" || arg == "--bible")
            {
                result.Bible = true;
            }
            else if (arg == "-r" || arg == "--recurse")
            {
                result.Recurse = true;
            }
            else if (arg == "-c" || arg == "--classic")
            {
                result.Classic = true;
            }
            else if (arg == "-j" || arg == "--json")
            {
                result.Json = true;
            }
            else if (arg == "-tr" || arg == "--tree")
            {
                result.Tree = true;
            }
            else if (arg == "-t" || arg == "--text")
            {
                result.Text = true;
            }
            else if (arg == "--hyprland-btw")
            {
                result.HyprlandBtw = true;
            }
            else if (arg.starts_with("-"))
            {
                fmt::println("Unknown option: {}", arg);
                result.Help = true;
            }
            else if (!result.Executable.has_value())
            {
                result.Executable = arg;
            }
            else
            {
                fmt::println("Unexpected argument: {}", arg);
                result.Help = true;
            }
        }

        if (result.Classic && result.Json)
        {
            fmt::println("Cannot use both --classic and --json, falling back to only --json");
            result.Classic = false;
        }

        return result;
    }
} // namespace dyneeded
