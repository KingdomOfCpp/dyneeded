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

namespace dyneeded {
    static const auto kSearchPaths = vector<fs::path>{
        "/lib", "/usr/lib", "/lib64", "/usr/lib64",
        "/lib/x86_64-linux-gnu", "/usr/lib/x86_64-linux-gnu"
    };

    optional<fs::path> FindLibrary(string_view name, span<const string> rpaths) {
        for (auto &rpath: rpaths) {
            if (auto p = fs::path(rpath) / name; fs::exists(p))
                return p;
        }
        if (auto env = getenv("LD_LIBRARY_PATH"); env) {
            stringstream ss(env);
            string token;
            while (getline(ss, token, ':'))
                if (auto p = fs::path(token) / name; fs::exists(p))
                    return p;
        }

        for (auto &dir: kSearchPaths)
            if (auto p = dir / name; fs::exists(p)) return p;

        return nullopt;
    }

    unordered_flat_map<string, vector<string> > GetVersions(const LIEF::ELF::Binary *binary) {
        auto result = unordered_flat_map<string, vector<string> >();
        for (const auto &req: binary->symbols_version_requirement()) {
            auto &versions = result[req.name()];
            for (const auto &aux: req.auxiliary_symbols()) {
                versions.push_back(aux.name());
            }
        }
        return result;
    }

    vector<string> GetRpaths(const LIEF::ELF::Binary *binary) {
        vector<string> rpaths;
        for (const auto &entry: binary->dynamic_entries()) {
            if (entry.tag() == LIEF::ELF::DynamicEntry::TAG::RPATH ||
                entry.tag() == LIEF::ELF::DynamicEntry::TAG::RUNPATH) {
                // downcase via base class
                if (const auto *rpath_entry = dynamic_cast<const LIEF::ELF::DynamicEntryRpath *>(&entry)) {
                    for (const auto &p: rpath_entry->paths())
                        rpaths.push_back(p);
                }
            }
        }
        return rpaths;
    }

    result<vector<DynamicLibrary> > ResolveDependencies(const fs::path &path) {
        auto elf = LIEF::ELF::Parser::parse(path);
        if (!elf)
            return new_error();

        auto versions = GetVersions(elf.get());
        auto rpaths = GetRpaths(elf.get());
        auto dependencies = vector<DynamicLibrary>();

        for (const auto &lib: elf->imported_libraries()) {
            auto path = FindLibrary(lib, rpaths);
            if (!path)
                return new_error();
            auto it = versions.find(lib);
            dependencies.push_back(DynamicLibrary{
                .Name = lib,
                .Path = *path,
                .Versions = (it != versions.end()) ? it->second : vector<string>{}
            });
        }

        return dependencies;
    }
}

#elif defined(DYNEEDED_WINDOWS)
#include <Windows.h>

namespace dyneeded {
    optional<fs::path> FindLibrary(string_view name) {
        char buffer[MAX_PATH];
        auto ret = SearchPathA(nullptr, name.data(), nullptr, MAX_PATH, buffer, nullptr);
        if (ret == 0 || ret > MAX_PATH) {
            return nullopt;
        }
        return fs::path(buffer);
    }

    result<vector<DynamicLibrary> > ResolveDependencies(const fs::path &path) {
        auto pe = LIEF::PE::Parser::parse(path.string());
        if (!pe)
            return new_error();

        auto dependencies = vector<DynamicLibrary>();
        for (const auto &lib: pe->imported_libraries()) {
            auto path = FindLibrary(lib);
            dependencies.push_back(DynamicLibrary{
                .Name = lib,
                .Path = path.value_or(fs::path(lib)),
                .Versions = vector<string>{}
            });
        }

        return dependencies;
    }
}

#else
#error "Unsupported platform"
#endif