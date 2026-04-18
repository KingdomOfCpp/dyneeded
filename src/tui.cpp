#include "tui.hpp"
#include "../core/prelude.hpp"
#include <fmt/base.h>
#include <fmt/format.h>
#include <iostream>
#include <memory> // for allocator, __shared_ptr_access
#include <string> // for char_traits, operator+, string, basic_string

#include "analyse.hpp"
#include "ftxui/component/captured_mouse.hpp" // for ftxui
#include "ftxui/component/component.hpp"
#include "ftxui/component/component.hpp"         // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"    // for ComponentBase
#include "ftxui/component/component_options.hpp" // for InputOption
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp" // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"     // for Ref

namespace dyneeded
{
using namespace ftxui;

int RunTuiMode(const Args &args)
{
    auto recursive = args.Recurse;
    auto radioboxList = vector<string>{
        "Fancy output", "Text output", "Classic output", "JSON output", "Tree output",
    };
    auto selected = 0;

    if (args.Text)
    {
        selected = 1;
    }
    else if (args.Classic)
    {
        selected = 2;
    }
    else if (args.Tree)
    {
        selected = 3;
    }
    else if (args.Json)
    {
        selected = 4;
    }

    auto screen = ScreenInteractive::FitComponent();
    auto exeName = args.Executable.value_or("");
    auto exeNameInput = Input(&exeName, "Executable name");
    auto recurseCheckbox = Checkbox("Recurse", &recursive);
    auto radiobox = Radiobox(radioboxList, &selected);
    auto runButton = Button("Run analysis", [&] {
        screen.ExitLoopClosure()(); // for some reason this function returns a function
    });

    auto exeRow = Renderer([&] { return hbox(text("Executable: "), exeNameInput->Render()); });

    auto component = Container::Vertical({
        exeRow,
        recurseCheckbox,
        radiobox,
        runButton,
    });

    screen.Loop(component);

    auto outputFormat = OutputFormat::Fancy;
    switch (selected)
    {
    case 0:
        outputFormat = OutputFormat::Fancy;
        break;
    case 1:
        outputFormat = OutputFormat::Text;
        break;
    case 2:
        outputFormat = OutputFormat::Classic;
        break;
    case 3:
        outputFormat = OutputFormat::Json;
        break;
    case 4:
        outputFormat = OutputFormat::Tree;
        break;
    default:
        fmt::println(stderr, "Invalid output format selected, defaulting to TUI");
        outputFormat = OutputFormat::Fancy;
        break;
    }

    RunAnalysis(args, outputFormat);

    return 0;
}

void PrintResultsTui(const Args &args, const AnalysisResult &result)
{
    auto elements = vector<Element>{};
    elements.push_back(text("Executable: " + args.Executable.value_or("")));
    elements.push_back(separator());
    elements.push_back(text("Shared objects:"));
    for (const auto &lib : result.Dependencies)
    {
        elements.push_back(text("\t" + lib.Name + " => " + lib.Path.string()));
    }
    if (result.MinimalGlibc || result.MinimalGlibcxx)
    {
        elements.push_back(separator());
        if (result.MinimalGlibc)
        {
            auto [major, minor] = *result.MinimalGlibc;
            elements.push_back(text(fmt::format("Minimal GLIBC version: {}.{}", major, minor)));
        }
        if (result.MinimalGlibcxx)
        {
            auto [major, minor] = *result.MinimalGlibcxx;
            elements.push_back(text(fmt::format("Minimal GLIBCXX version: {}.{}", major, minor)));
        }
    }

    auto doc = vbox(elements) | border;
    auto screen = Screen::Create(Dimension::Fit(doc));
    Render(screen, doc);
    std::cout << screen.ToString() << std::flush;
}
} // namespace dyneeded