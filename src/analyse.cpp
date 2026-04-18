#include "analyse.hpp"
#include "tui.hpp"
#include <fmt/base.h>
#include <glaze/glaze.hpp>
#include <span>

namespace dyneeded
{
static void TextOutput(const Args &args, const AnalysisResult &result)
{
    fmt::println("Executable: {}", *args.Executable);
    fmt::println("");
    fmt::println("Shared objects:");
    for (const auto &dep : result.Dependencies)
    {
        fmt::println("\t{} => {}", dep.Name, dep.Path.string());
    }
    fmt::println("");
    if (result.MinimalGlibc)
    {
        auto [glibcMajor, glibcMinor] = *result.MinimalGlibc;
        fmt::println("Minimal GLIBC version: {}.{}", glibcMajor, glibcMinor);
    }
    if (result.MinimalGlibcxx)
    {
        auto [glibcppMajor, glibcppMinor] = *result.MinimalGlibcxx;
        fmt::println("Minimal GLIBCXX version: {}.{}", glibcppMajor, glibcppMinor);
    }
}

static void ClassicOutput(const Args &args, const AnalysisResult &result)
{
    for (const auto &dep : result.Dependencies)
    {
        fmt::println("{} => {}", dep.Name, dep.Path.string());
    }
}

static void JsonOutput(const Args &args, const AnalysisResult &result)
{
    auto json = glz::write<glz::opts{.prettify = true}>(result);
    if (!json)
        fmt::println(stderr, "Error serializing to json");
    else
        fmt::println("{}", *json);
}

static void TreeOutput(const Args &args, const AnalysisResult &result)
{
    fmt::println(stderr, "Tree output format is not implemented yet, falling back to text output");
    TextOutput(args, result);
}

static void FancyOutput(const Args &args, const AnalysisResult &result)
{
    PrintResultsTui(args, result);
}

void RunAnalysis(const Args &args, OutputFormat outputFormat)
{
    if (!args.Executable)
    {
        fmt::println(stderr, "No executable provided, cannot run analysis");
        return;
    }

    auto analysis = AnalysisResult::AnalyzeExecutable(*args.Executable, args.Recurse);
    if (!analysis)
    {
        fmt::println(stderr, "Failed to resolve dependencies for {}", *args.Executable);
        return;
    }

    switch (outputFormat)
    {
    case OutputFormat::Fancy:
        FancyOutput(args, *analysis);
        break;

    case OutputFormat::Text:
        TextOutput(args, *analysis);
        break;

    case OutputFormat::Classic:
        ClassicOutput(args, *analysis);
        break;

    case OutputFormat::Json:
        JsonOutput(args, *analysis);
        break;

    case OutputFormat::Tree:
        TreeOutput(args, *analysis);
        break;
    }
}
} // namespace dyneeded