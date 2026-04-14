#include "resolver.hpp"
#include <boost/unordered/unordered_flat_map.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include "LIEF/Abstract/Binary.hpp"
#include "LIEF/ELF/Binary.hpp"
#include "LIEF/PE/Binary.hpp"
#include "LIEF/LIEF.hpp"
#include "dynamic_library.hpp"
#include "prelude.hpp"

#if defined(DYNEEDED_LINUX)

namespace dyneeded
{
    static const auto kSearchPaths = vector<fs::path>{
        "/lib", "/usr/lib", "/lib64", "/usr/lib64",
        "/lib/x86_64-linux-gnu", "/usr/lib/x86_64-linux-gnu"
    };

    optional<fs::path> FindLibrary(string_view name, span<const string> rpaths)
    {
        for (auto& rpath : rpaths)
        {
            if (auto p = fs::path(rpath) / name; fs::exists(p))
                return p;
        }
        if (auto env = getenv("LD_LIBRARY_PATH"); env)
        {
            stringstream ss(env);
            string token;
            while (getline(ss, token, ':'))
                if (auto p = fs::path(token) / name; fs::exists(p))
                    return p;
        }

        for (auto& dir : kSearchPaths)
            if (auto p = dir / name; fs::exists(p)) return p;

        return nullopt;
    }

    unordered_flat_map<string, vector<string>> GetVersions(const LIEF::ELF::Binary* binary)
    {
        auto result = unordered_flat_map<string, vector<string>>();
        for (const auto& req : binary->symbols_version_requirement())
        {
            auto& versions = result[req.name()];
            for (const auto& aux : req.auxiliary_symbols())
            {
                versions.push_back(aux.name());
            }
        }
        return result;
    }

    vector<string> GetRpaths(const LIEF::ELF::Binary* binary, const fs::path& path)
    {
        auto rpaths = vector<string>();
        rpaths.push_back(path.parent_path().string());
        // add the directory of the binary itself as the first search path
        for (const auto& entry : binary->dynamic_entries())
        {
            if (entry.tag() == LIEF::ELF::DynamicEntry::TAG::RPATH ||
                entry.tag() == LIEF::ELF::DynamicEntry::TAG::RUNPATH)
            {
                // downcase via base class
                if (const auto* rpath_entry = dynamic_cast<const LIEF::ELF::DynamicEntryRpath*>(&entry))
                {
                    for (const auto& p : rpath_entry->paths())
                        rpaths.push_back(p);
                }
            }
        }
        return rpaths;
    }

    static result<vector<DynamicLibrary>> ResolveDependenciesImpl(
        const fs::path& path,
        bool recurse,
        unordered_flat_map<string, bool>& visited)
    {
        auto elf = LIEF::ELF::Parser::parse(path);
        if (!elf)
            return new_error();

        auto versions = GetVersions(elf.get());
        auto rpaths = GetRpaths(elf.get(), path);
        auto dependencies = vector<DynamicLibrary>();

        for (const auto& lib : elf->imported_libraries())
        {
            if (visited.contains(lib))
                continue;
            visited[lib] = true;

            auto libPath = FindLibrary(lib, rpaths);
            if (!libPath)
                return new_error();

            auto it = versions.find(lib);
            dependencies.push_back(DynamicLibrary{
                .Name = lib,
                .Path = *libPath,
                .Versions = (it != versions.end()) ? it->second : vector<string>{}
            });

            if (recurse)
            {
                auto transitive = ResolveDependenciesImpl(*libPath, true, visited);
                if (!transitive)
                    return transitive; // propagate error
                for (auto& dep : *transitive)
                    dependencies.push_back(std::move(dep));
            }
        }

        return dependencies;
    }

    result<vector<DynamicLibrary>> ResolveDependencies(const fs::path& path, bool recurse)
    {
        auto visited = unordered_flat_map<string, bool>{};
        return ResolveDependenciesImpl(path, recurse, visited);
    }
}

#elif defined(DYNEEDED_WINDOWS)
#include <Windows.h>

namespace dyneeded
{
    optional<fs::path> FindLibrary(string_view name)
    {
        char buffer[MAX_PATH];
        auto ret = SearchPathA(nullptr, name.data(), nullptr, MAX_PATH, buffer, nullptr);
        if (ret == 0 || ret > MAX_PATH)
        {
            return nullopt;
        }
        return fs::path(buffer);
    }

        static result<vector<DynamicLibrary>> ResolveDependenciesImpl(
        const fs::path& path,
        bool recurse,
        unordered_flat_map<string, bool>& visited)
    {
        auto pe = LIEF::PE::Parser::parse(path.string());
        if (!pe)
            return new_error();

        auto dependencies = vector<DynamicLibrary>();
        for (const auto& lib : pe->imported_libraries())
        {
            if (visited.contains(lib))
                continue;
            visited[lib] = true;

            auto libPath = FindLibrary(lib);
            dependencies.push_back(DynamicLibrary{
                .Name = lib,
                .Path = libPath.value_or(fs::path(lib)),
                .Versions = vector<string>{}
            });

            if (recurse && libPath)
            {
                auto transitive = ResolveDependenciesImpl(*libPath, true, visited);
                if (!transitive)
                    return transitive;
                for (auto& dep : *transitive)
                    dependencies.push_back(std::move(dep));
            }
        }

        return dependencies;
    }

    result<vector<DynamicLibrary>> ResolveDependencies(const fs::path& path, bool recurse)
    {
        auto visited = unordered_flat_map<string, bool>{};
        return ResolveDependenciesImpl(path, recurse, visited);
    }
}

#else
#error "Unsupported platform"
#endif
