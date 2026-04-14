#include "analyse.hpp"
#include "tui.hpp"
#include <fmt/base.h>
#include <glaze/glaze.hpp>
#include <span>

namespace dyneeded {
    static void TextOutput(const Args& args,span<const DynamicLibrary> libraries) {
        fmt::println("Executable: {}", *args.Executable);
        fmt::println("");
        fmt::println("Shared objects:");
        for (const auto& dep : libraries) {
            fmt::println("\t{} => {}", dep.Name, dep.Path.string());
        }
    }

    static void ClassicOutput(const Args& args, span<const DynamicLibrary> libraries) {
        for (const auto& dep: libraries) {
            fmt::println("{} => {}", dep.Name, dep.Path.string());
        }
    }

    static void JsonOutput(const Args& args, span<const DynamicLibrary> libraries) {
        auto json = glz::write<glz::opts{.prettify = true}>(libraries);
        if (!json)
            fmt::println(stderr, "Error serializing to json");
        else
            fmt::println("{}", *json);
    }

    static void TreeOutput(const Args& args, span<const DynamicLibrary> libraries) {
        fmt::println(stderr, "Tree output format is not implemented yet, falling back to text output");
        TextOutput(args, libraries);
    }

    static void FancyOutput(const Args& args, span<const DynamicLibrary> libraries) {
        PrintResultsTui(args, libraries);
    }

    void RunAnalysis(const Args &args, OutputFormat outputFormat) {
        if (!args.Executable) {
            fmt::println(stderr, "No executable provided, cannot run analysis");
            return;
        }

        auto resolved = ResolveDependencies(*args.Executable, args.Recurse);
        if (!resolved) {
            fmt::println(stderr, "Failed to resolve dependencies for {}", *args.Executable);
            return;
        }

        switch (outputFormat) {
        case OutputFormat::Fancy:
            FancyOutput(args, *resolved);
            break;

        case OutputFormat::Text:
            TextOutput(args, *resolved);
            break;

        case OutputFormat::Classic:
            ClassicOutput(args, *resolved);
            break;

        case OutputFormat::Json:
            JsonOutput(args, *resolved);
            break;
            
        case OutputFormat::Tree:
            TreeOutput(args, *resolved);
            break;
        }
    }
}