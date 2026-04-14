#include "tui.hpp"
#include "../core/prelude.hpp"
#include <fmt/base.h>
#include <iostream>
#include <memory>  // for allocator, __shared_ptr_access
#include <string>  // for char_traits, operator+, string, basic_string
 
#include "analyse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"       // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"  // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"  // for Ref

namespace dyneeded {
    using namespace ftxui;

int RunTuiMode(const Args& args) {
    auto recursive = args.Recurse;
    auto radioboxList = vector<string>{
        "Fancy output",
        "Text output",
        "Classic output",
        "JSON output",
        "Tree output",
    };
    auto selected = 0;

    if (args.Json) {
        selected = 3;
    } else if (args.Classic) {
        selected = 2;
    } 

    auto screen = ScreenInteractive::FitComponent();
    auto exeName = args.Executable.value_or("");
    auto exeNameInput = Input(&exeName, "Executable name");
    auto recurseCheckbox = Checkbox("Recurse", &recursive);
    auto radiobox = Radiobox(radioboxList, &selected);
    auto runButton = Button("Run analysis", [&] {
        screen.ExitLoopClosure()(); // for some reason this function returns a function 
    });
 
    auto exeRow = Renderer([&] {
        return hbox(text("Executable: "), exeNameInput->Render());
    });

    auto component = Container::Vertical({
        exeRow,
        recurseCheckbox,
        radiobox,
        runButton,
    });
 
    screen.Loop(component);

    auto outputFormat = OutputFormat::Fancy;
    switch (selected) {
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

    void PrintResultsTui(const Args& args, span<const DynamicLibrary> libraries) {
        auto screen = ScreenInteractive::FitComponent();
        auto elements = vector<Element>{};
        elements.push_back(text("Executable: " + args.Executable.value_or("")));
        elements.push_back(separator());
        elements.push_back(text("Shared objects:"));
        for (const auto& lib : libraries) {
            elements.push_back(text("\t" + lib.Name + " => " + lib.Path.string()));
        }
        auto component = Renderer([&] {
            return vbox(elements) | border;
        });
        screen.Loop(component);
    }
}
