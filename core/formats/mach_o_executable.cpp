#include "mach_o_executable.hpp"

#include <LIEF/MachO.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <fmt/base.h>
#include <queue>
#include <unordered_set>
#include <fmt/core.h>

namespace dyneeded
{
    // Standard macOS dylib search paths, tried after rpaths and DYLD_LIBRARY_PATH.
    static const auto kSearchPaths = vector<fs::path>{
        "/usr/lib",
        "/usr/local/lib",
        "/System/Library/Frameworks",
        "/Library/Frameworks",
    };

    vector<string> MachOExecutable::GetRpaths(const LIEF::MachO::Binary* binary, const fs::path& path)
    {
        auto rpaths = vector<string>();

        // essentially mirror elf behaviour
        rpaths.push_back(path.parent_path().string());

        for (const auto& cmd : binary->commands())
        {
            if (cmd.command() == LIEF::MachO::LoadCommand::TYPE::RPATH)
            {
                if (const auto* rp = dynamic_cast<const LIEF::MachO::RPathCommand*>(&cmd))
                {
                    auto p = string(rp->path());
                    constexpr string_view kExec = "@executable_path";
                    constexpr string_view kLoader = "@loader_path";
                    if (p.starts_with(kExec))
                        p = rpaths[0] + p.substr(kExec.size());
                    else if (p.starts_with(kLoader))
                        p = rpaths[0] + p.substr(kLoader.size());

                    rpaths.push_back(std::move(p));
                }
            }
        }
        return rpaths;
    }

    optional<fs::path> MachOExecutable::FindLibrary(string_view name, span<const string> rpaths)
    {
        constexpr string_view kRpath      = "@rpath/";
        constexpr string_view kExecPath   = "@executable_path/";
        constexpr string_view kLoaderPath = "@loader_path/";

        auto tryPath = [](const fs::path& p) -> optional<fs::path>
        {
            if (fs::exists(p))
                return p;
            return nullopt;
        };

        if (name.starts_with(kRpath))
        {
            auto tail = name.substr(kRpath.size());
            for (const auto& rp : rpaths)
                if (auto p = tryPath(fs::path(rp) / tail))
                    return p;
            return nullopt;
        }

        if (name.starts_with(kExecPath))
            return tryPath(fs::path(rpaths.empty() ? "" : rpaths[0]) / name.substr(kExecPath.size()));

        if (name.starts_with(kLoaderPath))
            return tryPath(fs::path(rpaths.empty() ? "" : rpaths[0]) / name.substr(kLoaderPath.size()));

        if (name.starts_with(kRpath))
        {
            // iterate every @rpath entry
            auto tail = name.substr(kRpath.size());
            for (const auto& rp : rpaths)
                if (auto p = tryPath(fs::path(rp) / tail))
                    return p;
            return nullopt;
        }

        // absolute install name (mostly for system libs)
        if (name.starts_with('/'))
        {
            if (auto p = tryPath(fs::path(name)))
                return p;
        }

        auto filename = fs::path(name).filename();

        for (const auto& rp : rpaths)
            if (auto p = tryPath(fs::path(rp) / filename))
                return p;

#ifdef DYNEEDED_MACOS
        if (const char* env = getenv("DYLD_LIBRARY_PATH"))
        {
            auto ss = stringstream(env);
            auto token = string();
            while (getline(ss, token, ':'))
                if (auto p = tryPath(fs::path(token) / filename))
                    return p;
        }
#endif

        for (const auto& dir : kSearchPaths)
        {
            if (auto p = tryPath(dir / filename))
                return p;
            // Also try .framework bundles: Frameworks/Foo.framework/Foo
            auto stem = filename.stem();
            if (auto p = tryPath(dir / (stem.string() + ".framework") / stem))
                return p;
        }

        return nullopt;
    }

    /// Pull version strings out of the Mach-O binary's LC_VERSION_MIN_* and
    /// LC_BUILD_VERSION commands.  The strings are stored in the same format
    /// used by the ELF backend so that VersionInfo::Parse can handle them.
    static vector<string> CollectNeededVersions(const LIEF::MachO::Binary* binary)
    {
        auto versions = vector<string>();

        for (const auto& cmd : binary->commands())
        {
            using TYPE = LIEF::MachO::LoadCommand::TYPE;

            if (cmd.command() == TYPE::BUILD_VERSION)
            {
                if (const auto* bv = dynamic_cast<const LIEF::MachO::BuildVersion*>(&cmd))
                {
                    const auto v = bv->minos();
                    versions.push_back(fmt::format("macOS-{}.{}.{}", v[0], v[1], v[2]));
                }
            }
            else if (cmd.command() == TYPE::VERSION_MIN_MACOSX)
            {
                if (const auto* vm = dynamic_cast<const LIEF::MachO::VersionMin*>(&cmd))
                {
                    const auto v = vm->version();
                    versions.push_back(fmt::format("macOS-{}.{}.{}", v[0], v[1], v[2]));
                }
            }
            else if (cmd.command() == TYPE::VERSION_MIN_IPHONEOS)
            {
                if (const auto* vm = dynamic_cast<const LIEF::MachO::VersionMin*>(&cmd))
                {
                    const auto v = vm->version();
                    versions.push_back(fmt::format("iPhoneOS-{}.{}.{}", v[0], v[1], v[2]));
                }
            }
        }

        return versions;
    }

    MachOExecutable MachOExecutable::AnalyzeMachOExecutable(const LIEF::MachO::Binary* binary,
                                                            string_view name,
                                                            span<const string> rpaths,
                                                            optional<fs::path> path,
                                                            unordered_set<string>& visited)
    {
        auto deps = vector<MachOExecutable>();

        for (const auto& cmd : binary->commands())
        {
            if (cmd.command() != LIEF::MachO::LoadCommand::TYPE::LOAD_DYLIB &&
                cmd.command() != LIEF::MachO::LoadCommand::TYPE::LOAD_WEAK_DYLIB &&
                cmd.command() != LIEF::MachO::LoadCommand::TYPE::REEXPORT_DYLIB)
                continue;

            const auto* dylib_cmd = dynamic_cast<const LIEF::MachO::DylibCommand*>(&cmd);
            if (!dylib_cmd)
                continue;

            const string rawName = dylib_cmd->name();
            if (rawName.empty())
                continue;

            auto foundLib = FindLibrary(rawName, rpaths);
            if (!foundLib)
            {
                deps.push_back(MachOExecutable(rawName, nullopt, {}, nullopt, {}));
                continue;
            }

            auto ec = error_code();
            auto canonical = fs::canonical(*foundLib, ec).string();
            if (ec)
                canonical = foundLib->string();

            if (visited.contains(canonical))
                continue;
            visited.insert(canonical); // mark BEFORE recursing to break cycles

            // LIEF parses fat binaries as a FatBinary containing multiple slices;
            // pick the first (and usually only) slice for analysis.
            auto fat = LIEF::MachO::Parser::parse(foundLib->string());
            if (!fat || fat->size() == 0)
            {
                fmt::println(stderr, "Failed to parse Mach-O that should have been valid: '{}'",
                             foundLib->string());
                deps.push_back(MachOExecutable(foundLib->filename().string(), *foundLib, {}, nullopt, {}));
                continue;
            }

            const LIEF::MachO::Binary* child = fat->at(0);
            auto childRpaths = GetRpaths(child, *foundLib);

            deps.push_back(AnalyzeMachOExecutable(child, foundLib->filename().string(),
                                                  childRpaths, *foundLib, visited));
        }

        auto neededVersions = CollectNeededVersions(binary);

        return {string(name), std::move(path), std::move(deps), nullopt, std::move(neededVersions)};
    }

    result<MachOExecutable> MachOExecutable::FromBinary(const fs::path& file, const LIEF::MachO::Binary* macho)
    {
        auto ec = error_code();
        auto canonical = fs::canonical(file, ec).string();
        if (ec)
            canonical = file.string();

        auto rpaths = GetRpaths(macho, file);
        auto visited = unordered_set<string>{canonical};
        return AnalyzeMachOExecutable(macho, file.filename().string(), rpaths, file, visited);
    }

    unordered_flat_map<string, VersionInfo>
    MachOExecutable::GetMinimumRequiredVersionsImpl(const MachOExecutable& executable)
    {
        auto maxByPrefix = unordered_flat_map<string, VersionInfo>();

        auto q = queue<const MachOExecutable*>();
        q.push(&executable);

        while (!q.empty())
        {
            auto node = q.front();
            q.pop();

            for (const auto& versionStr : node->GetNeededVersions())
            {
                auto parsed = VersionInfo::Parse(versionStr);
                if (!parsed)
                    continue;

                auto it = maxByPrefix.find(parsed->Prefix);
                if (it == maxByPrefix.end() || parsed->Parts > it->second.Parts)
                    maxByPrefix[parsed->Prefix] = *parsed;
            }

            for (const auto& dep : node->GetDirectDependencies())
                q.push(&dep);
        }

        return maxByPrefix;
    }

    unordered_flat_map<string, VersionInfo> MachOExecutable::GetMinimumRequiredVersions() const
    {
        return GetMinimumRequiredVersionsImpl(*this);
    }

    unordered_flat_map<string, VersionInfo> MachOExecutable::GetMinimumDirectlyRequiredVersions() const
    {
        auto maxByPrefix = boost::unordered_flat_map<string, VersionInfo>();

        for (const auto& versionStr : neededVersions)
        {
            auto parsed = VersionInfo::Parse(versionStr);
            if (!parsed)
                continue;

            auto it = maxByPrefix.find(parsed->Prefix);
            if (it == maxByPrefix.end() || parsed->Parts > it->second.Parts)
                maxByPrefix[parsed->Prefix] = *parsed;
        }

        return maxByPrefix;
    }

    string_view MachOExecutable::GetName() const
    {
        return name;
    }

    const fs::path* MachOExecutable::GetPath() const
    {
        return path ? &*path : nullptr;
    }

    span<const MachOExecutable> MachOExecutable::GetDirectDependencies() const
    {
        return directDependencies;
    }

    vector<MachOExecutable> MachOExecutable::GetDependencies() const
    {
        auto result = vector<MachOExecutable>();
        auto visited = unordered_set<string>();
        auto q = queue<const MachOExecutable*>();

        for (const auto& dep : directDependencies)
            q.push(&dep);

        while (!q.empty())
        {
            auto node = q.front();
            q.pop();

            auto key = node->path
                           ? fs::weakly_canonical(*node->path).string()
                           : string(node->GetName());

            if (!visited.insert(key).second)
                continue;

            result.push_back(*node);

            for (const auto& dep : node->directDependencies)
                q.push(&dep);
        }

        return result;
    }

    optional<string_view> MachOExecutable::GetVersion() const
    {
        return version;
    }

    const vector<string>& MachOExecutable::GetNeededVersions() const
    {
        return neededVersions;
    }

    MachOExecutable::MachOExecutable(string name, optional<fs::path> path,
                                     vector<MachOExecutable> directDependencies,
                                     optional<string> version,
                                     vector<string> neededVersions)
        : name(std::move(name)), path(std::move(path)),
          directDependencies(std::move(directDependencies)),
          version(std::move(version)),
          neededVersions(std::move(neededVersions))
    {
    }
} // namespace dyneeded
