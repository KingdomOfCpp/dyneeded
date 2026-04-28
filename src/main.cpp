#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <span>

#include "boost/algorithm/string/split.hpp"
#include <boost/algorithm/string.hpp>
#include <fmt/base.h>
#include <fmt/ranges.h>
#include <glaze/glaze.hpp>
#include <vector>

#include "args.hpp"
#include "bible.hpp"
#include "core/formats/elf_executable.hpp"
#include "core/print/print_tree.hpp"
#include "glaze/core/write.hpp"
#include "glaze/json/write.hpp"

#include <LIEF/Abstract.hpp>
#include <LIEF/Abstract/Parser.hpp>
#include <LIEF/ELF/Binary.hpp>
#include <LIEF/MachO/Binary.hpp>
#include <LIEF/PE/Binary.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#include "core/formats/mach_o_executable.hpp"
#include "core/formats/portable_executable.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace dyneeded;

static constexpr auto kVersionString = "dyneeded v2.1.0";

static constexpr auto kHelpMessage = "Usage: dyneeded <executable> [options]\n"
    "Available options:\n"
    "\t-j or --json to get the results in json\n"
    "\t-tr or --tree to get the output in a tree format\n"
    "\t-t or --text to get the output in a less fancy although readable format\n"
    "\t-c or --classic for classic ldd style printing\n"
    "\tMore hidden!\n";

template <typename TExecutable>
void PrintText(const TExecutable& executable, const Args& args)
{
    fmt::println("Executable:\n\t{}", *args.Executable);
    fmt::println("");
    fmt::println("Dynamic dependencies:");
    for (const auto& dep : executable.GetDependencies())
    {
        if (auto path = dep.GetPath())
            fmt::println("\t{} => {}", dep.GetName(), path->string());
        else
            fmt::println("\t{}", dep.GetName());
    }
    fmt::println("");
    fmt::println("Minimum version of libs:");
    for (const auto& [name, version] : executable.GetMinimumDirectlyRequiredVersions())
    {
        fmt::println("\t{}: {}", version.Prefix, fmt::join(version.Parts, "."));
    }
}

template <typename TExecutable>
void PrintClassic(const TExecutable& executable)
{
    for (const auto& dep : executable.GetDependencies())
    {
        if (auto path = dep.GetPath())
            fmt::println("\t{} => {}", dep.GetName(), path->string());
        else
            fmt::println("\t{}", dep.GetName());
    }
}

template <typename TExecutable>
void PrintFancy(const TExecutable& executable, const Args& args)
{
    using namespace ftxui;

    auto maxNameLen = size_t(0);
    for (const auto& dep : executable.GetDependencies())
        maxNameLen = std::max(maxNameLen, dep.GetName().size());

    auto depRows = vector<Element>();
    for (const auto& dep : executable.GetDependencies())
    {
        auto name = text(string(dep.GetName())) | color(Color::Cyan);
        auto nameCell = name | size(WIDTH, EQUAL, static_cast<int>(maxNameLen));

        auto row = Element();
        if (auto path = dep.GetPath())
            row = hbox({nameCell, text(" => ") | dim, text(path->string()) | color(Color::Green) | flex});
        else
            row = hbox({nameCell, text("  (not found)") | color(Color::RedLight) | dim});

        depRows.push_back(hbox({text("  "), row}) | flex);
    }

    auto maxPrefixLen = size_t(0);
    for (const auto& [name, version] : executable.GetMinimumDirectlyRequiredVersions())
        maxPrefixLen = std::max(maxPrefixLen, version.Prefix.size());

    auto versionRows = vector<Element>();
    for (const auto& [name, version] : executable.GetMinimumDirectlyRequiredVersions())
    {
        auto label = text(version.Prefix) | bold | size(WIDTH, EQUAL, (int)maxPrefixLen);
        auto ver = text(fmt::format("{}", fmt::join(version.Parts, "."))) | color(Color::Yellow);
        versionRows.push_back(hbox({text("  "), label, text("  "), ver}));
    }

    auto sectionHeader = [](const std::string& title)
    {
        return text(title) | bold | color(Color::White);
    };

    auto doc =
        !versionRows.empty()
            ? vbox({
                hbox({
                    text(" Executable: ") | dim,
                    text(fs::path(*args.Executable).filename().string()) | bold | color(Color::Cyan)
                }),
                separator(),
                hbox({text(" "), sectionHeader("Dynamic dependencies")}),
                vbox(depRows),
                separator(),
                hbox({text(" "), sectionHeader("Minimum directly required versions")}),
                vbox(versionRows),
            }) |
            border
            : vbox({
                hbox({
                    text(" Executable: ") | dim,
                    text(fs::path(*args.Executable).filename().string()) | bold | color(Color::Cyan)
                }),
                separator(),
                hbox({text(" "), sectionHeader("Dynamic dependencies")}),
                vbox(depRows),
            }) |
            border;

    auto screen = Screen::Create(Dimension::Fit(doc), Dimension::Fit(doc));
    Render(screen, doc);
    screen.Print();
}

template <typename TExecutable>
int RunRegular(const TExecutable& executable, const Args& args)
{
    if (args.Text)
    {
        PrintText<TExecutable>(executable, args);
    }
    else if (args.Classic)
    {
        PrintClassic<TExecutable>(executable);
    }
    else if (args.Json)
    {
        auto result = glz::write<glz::opts{.prettify = true}>(executable);
        if (!result)
        {
            fmt::println(stderr, "Failed to serialize to JSON");
            return 1;
        }
        fmt::println("{}", *result);
    }
    else if (args.Tree)
    {
        PrintTree(executable);
    }
    else
    {
        PrintFancy(executable, args);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    auto rawArgs = vector<string>(argv + 1, argv + argc);
    auto args = Args::Parse(rawArgs);

    if (args.Help)
    {
        fmt::println("{}", kHelpMessage);
        return 0;
    }
    else if (args.Bible)
    {
        srand(chrono::high_resolution_clock::now().time_since_epoch().count());
        auto biblePassage = kBiblePassages[rand() % std::size(kBiblePassages)];
        fmt::println("{}", biblePassage);
        return 0;
    }
    else if (args.HyprlandBtw)
    {
        fmt::println(stderr, "HyprLARP is not supported yet :(");
        return 1;
    }
    else if (args.Version)
    {
        fmt::println("{}", kVersionString);
        return 0;
    }
    else if (!args.Executable)
    {
        fmt::println(stderr, "No executable provided");
        fmt::println("{}", kHelpMessage);
        return 1;
    }

    return try_handle_all(
        [&]() -> result<int>
        {
            if (!fs::exists(*args.Executable))
            {
                fmt::println(stderr, "'{}' does not exist", *args.Executable);
                return 1;
            }

            if (!fs::is_regular_file(*args.Executable))
            {
                fmt::println(stderr, "'{}' is not a regular file", *args.Executable);
                return 1;
            }

            auto binary = LIEF::Parser::parse(*args.Executable);
            if (auto elfRaw = dynamic_cast<LIEF::ELF::Binary*>(binary.get()))
            {
#ifndef DYNEEDED_LINUX
                fmt::println("ELF path resolution is limited on non-linux platforms");
#endif
                BOOST_LEAF_AUTO(elf, ElfExecutable::FromBinary(*args.Executable, elfRaw));
                return RunRegular(elf, args);
            }
            else if (auto machoRaw = dynamic_cast<LIEF::MachO::Binary*>(binary.get()))
            {
#ifndef DYNEEDED_MACOS
                fmt::println("Mach-O path resolution is limited on non-macos platforms");
#endif
                BOOST_LEAF_AUTO(macho, MachOExecutable::FromBinary(*args.Executable, machoRaw));
                return RunRegular(macho, args);
            }
            else if (auto rawPe = dynamic_cast<LIEF::PE::Binary*>(binary.get()))
            {
#ifndef DYNEEDED_WINDOWS
                fmt::println("PE path resolution is limited on non-windows platforms");
#endif
                BOOST_LEAF_AUTO(pe, PeExecutable::FromBinary(*args.Executable, rawPe));
                return RunRegular(pe, args);
            }
            return 0;
        },
        [](FailedToParseElfError e)
        {
            fmt::println("Failed to read the elf executable");
            return 1;
        },
        [](const exception& e)
        {
            fmt::println(stderr, "Exception: {}", e.what());
            return 1;
        },
        []()
        {
            fmt::println(stderr, "Unknown error");
            return 1;
        });

    return 0;
}
