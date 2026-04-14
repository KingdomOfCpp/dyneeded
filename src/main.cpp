#include <cstddef>
#include <span>

#include "../core/resolver.hpp"
#include "analyse.hpp"
#include "boost/algorithm/string/split.hpp"
#include <boost/algorithm/string.hpp>
#include <vector>
#include <glaze/glaze.hpp>
#include <fmt/base.h>

#include "args.hpp"
#include "glaze/core/write.hpp"
#include "glaze/json/write.hpp"
#include "tui.hpp"

using namespace dyneeded;

static constexpr auto kHelpMessage = "Usage: dyneeded <executable> [options]\n"
        "Available options:\n"
        "\t-r or --recurse to also get the deps of the deps\n"
        "\t-j or --json to get the results in json\n"
        "\t-c or --classic for classic ldd style printing\n"
        "\tMore hidden!\n";

static constexpr string_view kBiblePassages[] = {
    "Once, on being asked by the Pharisees when the kingdom of God would come, Jesus replied, "
    "“The coming of the kingdom of God is not something that can be observed, nor will people say, 'Here it is' "
    "or 'There it is' because the kingdom of God is in your midst.”",
};

static int MainMode(const Args& args) {
    auto outputFormat = OutputFormat::Text;
    if (args.Json) {
        outputFormat = OutputFormat::Json;
    } else if (args.Classic) {
        outputFormat = OutputFormat::Classic;
    } else if (args.Tree) {
        outputFormat = OutputFormat::Tree;
    } else if (args.Fancy) {
        outputFormat = OutputFormat::Fancy;
    }

    RunAnalysis(args, outputFormat);

    return 0;
}

int main(int argc, char *argv[]) {
    auto raw_args = vector<string>(argv + 1, argv + argc);
    auto args = Args::Parse(raw_args);

    if (args.Help) {
        fmt::println("{}", kHelpMessage);
    } else if (args.Bible) {
        auto biblePassage = kBiblePassages[rand() % std::size(kBiblePassages)];
        fmt::println("{}", biblePassage);
    } else if (args.HyprlandBtw) {
        return RunTuiMode(args);
    } else if (!args.Executable) {
        fmt::println(stderr, "Must supply an executable");
        return 1;
    } else {
        return MainMode(args);
    }

    return 0;
}
