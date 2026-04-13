#include <cstddef>
#include <span>

#include "../core/resolver.hpp"
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
    auto exe = *args.Executable;
    if (args.Recurse) {
        fmt::println(stderr, "Recurse not implemented yet");
    }
    auto exe_path = fs::path(exe);
    auto deps = ResolveDependencies(exe_path);
    if (deps.has_error()) {
        fmt::println(stderr, "Error resolving dependencies");
        return 1;
    }

    if (args.Json) {
        auto json = glz::write<glz::opts{.prettify = true}>(*deps);
        if (json) {
            fmt::println("{}", *json);
        } else {
            fmt::println(stderr, "Error serializing to json");
            return 1;
        }
    } else if (args.Classic) {
        for (const auto& dep: *deps) {
            fmt::println("{} => {}", dep.Name, dep.Path.string());
        }
    } else {
        fmt::println("Executable: {}", absolute(exe_path).string());
        fmt::println("");
        fmt::println("Shared objects:");
        for (const auto& dep : *deps) {
            fmt::println("\t{} => {}", dep.Name, dep.Path.string());
        }
    }

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
        return RunTuiMode();
    } else if (!args.Executable) {
        fmt::println(stderr, "Must supply an executable");
        return 1;
    } else {
        return MainMode(args);
    }

    return 0;
}
