#include "elf_executable.hpp"

#include <LIEF/ELF.hpp>
#include <fmt/base.h>
#include <unordered_set>

namespace dyneeded
{
static const auto kSearchPaths =
    vector<fs::path>{"/lib", "/usr/lib", "/lib64", "/usr/lib64", "/lib/x86_64-linux-gnu", "/usr/lib/x86_64-linux-gnu"};

vector<string> GetRpaths(const LIEF::ELF::Binary *binary, const fs::path &path)
{
    auto rpaths = vector<string>();
    rpaths.push_back(path.parent_path().string());
    // add the directory of the binary itself as the first search path
    for (const auto &entry : binary->dynamic_entries())
    {
        if (entry.tag() == LIEF::ELF::DynamicEntry::TAG::RPATH || entry.tag() == LIEF::ELF::DynamicEntry::TAG::RUNPATH)
        {
            // downcase via base class
            if (const auto *rpath_entry = dynamic_cast<const LIEF::ELF::DynamicEntryRpath *>(&entry))
            {
                for (const auto &p : rpath_entry->paths())
                    rpaths.push_back(p);
            }
        }
    }
    return rpaths;
}

optional<fs::path> FindLibrary(string_view name, span<const string> rpaths)
{
    for (auto &rpath : rpaths)
    {
        if (auto p = fs::path(rpath) / name; fs::exists(p))
            return p;
    }
#ifdef DYNEEDED_LINUX
    if (auto env = getenv("LD_LIBRARY_PATH"); env)
    {
        stringstream ss(env);
        string token;
        while (getline(ss, token, ':'))
            if (auto p = fs::path(token) / name; fs::exists(p))
                return p;
    }

    for (auto &dir : kSearchPaths)
        if (auto p = dir / name; fs::exists(p))
            return p;
#endif

    return nullopt;
}

ElfExecutable ElfExecutable::AnalyzeElfExecutable(const LIEF::ELF::Binary *binary, string_view name,
                                                  span<const string> rpaths, optional<fs::path> path,
                                                  unordered_set<string> &visited)
{
    auto deps = vector<ElfExecutable>();
    for (const auto &lib : binary->imported_libraries())
    {
        if (auto foundLib = FindLibrary(lib, rpaths))
        {
            auto canonical = fs::canonical(*foundLib).string();
            if (visited.contains(canonical)) // skip already seen
                continue;
            visited.insert(canonical); // mark BEFORE recursing

            auto childRpaths = GetRpaths(LIEF::ELF::Parser::parse(*foundLib).get(), *foundLib);

            auto elf = LIEF::ELF::Parser::parse(*foundLib);
            if (!elf)
            {
                fmt::println(stderr, "Failed to parse ELF that should have been valid: '{}'", foundLib->string());
                continue;
            }

            deps.push_back(
                AnalyzeElfExecutable(elf.get(), foundLib->filename().string(), childRpaths, *foundLib, visited));
        }
        else
        {
            deps.push_back(ElfExecutable(lib, nullopt, {}, nullopt, {}));
        }
    }

    auto versionDeps = vector<string>();
    for (const auto& dep : binary->symbols_version_requirement())
    {
        for (const auto& aux : dep.auxiliary_symbols())
        {
            versionDeps.push_back(aux.name());
        }
    }

    return {string(name), std::move(path), std::move(deps), nullopt, move(versionDeps)};
}

result<ElfExecutable> ElfExecutable::FromFile(const fs::path &file)
{
    auto elf = LIEF::ELF::Parser::parse(file);
    if (!elf)
        return new_error(FailedToParseElfError());

    auto rpaths = GetRpaths(elf.get(), file);
    auto visited = unordered_set<string>{fs::canonical(file).string()};

#ifndef DYNEEDED_LINUX
    fmt::println("ELF support is limited on non linux platforms");
#endif

    return AnalyzeElfExecutable(elf.get(), file.filename().string(), rpaths, file, visited);
}

string_view ElfExecutable::GetName() const
{
    return name;
}

const fs::path *ElfExecutable::GetPath() const
{
    return path ? &*path : nullptr;
}

span<const ElfExecutable> ElfExecutable::GetDependencies() const
{
    return dynamicDependencies;
}

optional<string_view> ElfExecutable::GetVersion() const
{
    return version;
}

const vector<string> &ElfExecutable::GetNeededVersions() const
{
    return neededVersions;
}

ElfExecutable::ElfExecutable(string name, optional<fs::path> path, vector<ElfExecutable> dynamic_dependencies,
                             optional<string> version, vector<string> neededVersions)
    : name(std::move(name)), path(std::move(path)), dynamicDependencies(std::move(dynamic_dependencies)),
      version(std::move(version)), neededVersions(std::move(neededVersions))
{
}
} // namespace dyneeded
