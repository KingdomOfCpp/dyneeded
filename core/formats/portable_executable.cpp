#include "portable_executable.hpp"

#include <LIEF/PE.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <queue>
#include <regex>
#include <unordered_set>

#ifdef DYNEEDED_WINDOWS
#include <Windows.h>
#endif

namespace dyneeded
{
    static const auto kSearchPaths = vector<fs::path>{
        "C:/Windows/System32",
        "C:/Windows/SysWOW64",
        "C:/Windows",
    };

    optional<VersionInfo> VersionFromResource(const fs::path& dllPath)
    {
        auto pe = LIEF::PE::Parser::parse(dllPath.string());
        if (!pe)
            return nullopt;

        auto mgr_result = pe->resources_manager();
        if (!mgr_result)
            return nullopt;

        const auto& mgr = *mgr_result;
        if (!mgr.has_version())
            return nullopt;

        const auto versions = mgr.version();
        if (versions.empty())
            return nullopt;

        // Take the first VS_VERSIONINFO entry (virtually always exactly one)
        if (versions.size() < 1)
            return nullopt;
        const auto& ffi = versions[0].file_info();

        // validate the magic just to make sure
        if (ffi.signature != LIEF::PE::ResourceVersion::fixed_file_info_t::SIGNATURE_VALUE)
            return nullopt;

        // file_version_ms: high word = major, low word = minor
        // file_version_ls: high word = patch, low word = build
        const uint16_t major = static_cast<uint16_t>(ffi.file_version_ms >> 16);
        const uint16_t minor = static_cast<uint16_t>(ffi.file_version_ms & 0xFFFF);
        const uint16_t patch = static_cast<uint16_t>(ffi.file_version_ls >> 16);

        // Use the DLL stem as the prefix, e.g. "VCRUNTIME140" -> "VCRUNTIME140-14.0.9"
        auto stem = dllPath.stem().string();
        for (auto& c : stem)
            c = static_cast<char>(toupper(c));

        return VersionInfo{stem, {major, minor, patch}};
    }

    optional<VersionInfo> VersionFromImportName(string_view rawName)
    {
        // standardize to uppercased, extension-stripped
        auto name = string(rawName);
        for (auto& c : name)
            c = static_cast<char>(toupper(c));
        if (auto dot = name.rfind('.'); dot != string::npos)
            name.erase(dot);

        // match against the classic MSVC CRT / STL DLLs
        // Format: <PREFIX><version>[_<suffix>]
        // e.g.    MSVCR140, MSVCP140_1, VCRUNTIME140_1
        static const auto kMsvc = regex(R"(^(MSVCR|MSVCP|VCRUNTIME|CONCRT|VCCORLIB|MSVCM)(\d+)(?:_(\d+))?$)");
        auto m = smatch();
        if (regex_match(name, m, kMsvc))
        {
            auto parts = m[3].matched
                             ? vector<int>{stoi(m[2].str()), stoi(m[3].str())}
                             : vector<int>{stoi(m[2].str())};
            return VersionInfo{m[1].str(), std::move(parts)};
        }


        return nullopt;
    }

    optional<fs::path> PeExecutable::FindLibrary(string_view name, const fs::path& searchRoot)
    {
#ifdef DYNEEDED_WINDOWS
        auto nameStr = string(name);

        // 1st pass searchRoot prepended
        {
            TCHAR result[MAX_PATH];
            if (SearchPathA(searchRoot.string().c_str(), nameStr.c_str(), nullptr, MAX_PATH, result, nullptr) > 0)
                return fs::path(result);
        }

        // 2nd pass standard order (SearchPath with lpPath = NULL)
        {
            TCHAR result[MAX_PATH];
            if (SearchPathA(nullptr, nameStr.c_str(), nullptr, MAX_PATH, result, nullptr) > 0)
                return fs::path(result);
        }

        return nullopt;

#else
        // on linux and mac search next to the exe or in PATH
        if (auto p = searchRoot / name; fs::exists(p))
            return p;

        if (const char* env = getenv("PATH"))
        {
            auto ss = stringstream(env);
            auto token = string();
            while (getline(ss, token, ':'))
                if (auto p = fs::path(token) / name; fs::exists(p))
                    return p;
        }

        for (const auto& dir : kSearchPaths)
            if (auto p = dir / name; fs::exists(p))
                return p;

        return nullopt;
#endif
    }

    PeExecutable PeExecutable::AnalyzePeExecutable(const LIEF::PE::Binary* binary, string_view name,
                                                   const fs::path& searchRoot, optional<fs::path> path,
                                                   unordered_set<string>& visited)
    {
        auto deps = vector<PeExecutable>();

        for (const auto& imp : binary->imports())
        {
            const string& dllName = imp.name();
            if (dllName.empty())
                continue;

            // these are fake dlls that are not interesting to report on
            if (dllName.starts_with("api-ms-") || dllName.starts_with("ext-ms-"))
                continue;

            auto foundLib = FindLibrary(dllName, searchRoot);
            if (!foundLib)
            {
                // Record as an unresolved dependency (no path)
                deps.push_back(PeExecutable(dllName, nullopt, {}));
                continue;
            }

            // Canonicalize to deduplicate symlinks / relative paths
            auto ec = error_code();
            auto canonical = fs::canonical(*foundLib, ec).string();
            if (ec)
                canonical = foundLib->string(); // fall back if canonicalisation fails

            if (visited.contains(canonical))
                continue;
            visited.insert(canonical); // mark BEFORE recursing to break cycles

            auto child = LIEF::PE::Parser::parse(foundLib->string());
            if (!child)
            {
                fmt::println(stderr, "Failed to parse PE that should have been valid: '{}'", foundLib->string());
                deps.push_back(PeExecutable(dllName, *foundLib, {}));
                continue;
            }

            // The child's search root is its own directory
            const auto childRoot = foundLib->parent_path();
            deps.push_back(AnalyzePeExecutable(child.get(), foundLib->filename().string(), childRoot, *foundLib,
                                               visited));
        }

        return {string(name), std::move(path), std::move(deps)};
    }

    result<PeExecutable> PeExecutable::FromBinary(const fs::path& file, const LIEF::PE::Binary* pe)
    {
        auto ec = error_code();
        auto canonical = fs::canonical(file, ec).string();
        if (ec)
            canonical = file.string();

        auto visited = unordered_set<string>{canonical};
        const auto searchRoot = file.parent_path();

        return AnalyzePeExecutable(pe, file.filename().string(), searchRoot, file, visited);
    }

    string_view PeExecutable::GetName() const
    {
        return name;
    }

    const fs::path* PeExecutable::GetPath() const
    {
        return path ? &*path : nullptr;
    }

    span<const PeExecutable> PeExecutable::GetDirectDependencies() const
    {
        return directDependencies;
    }

    vector<PeExecutable> PeExecutable::GetDependencies() const
    {
        auto result = vector<PeExecutable>();
        auto visited = unordered_set<string>();
        auto q = queue<const PeExecutable*>();

        for (const auto& dep : directDependencies)
            q.push(&dep);

        while (!q.empty())
        {
            auto node = q.front();
            q.pop();

            auto key = node->path ? fs::weakly_canonical(*node->path).string() : string(node->GetName());

            if (!visited.insert(key).second)
                continue;

            result.push_back(*node);

            for (const auto& dep : node->directDependencies)
                q.push(&dep);
        }

        return result;
    }

    static unordered_flat_map<string, VersionInfo> CollectVersionsForNode(const PeExecutable& node)
    {
        auto result = unordered_flat_map<string, VersionInfo>();

        auto consider = [&](optional<VersionInfo> parsed)
        {
            if (!parsed) return;
            auto it = result.find(parsed->Prefix);
            if (it == result.end() || parsed->Parts > it->second.Parts)
                result[parsed->Prefix] = *parsed;
        };

        for (const auto& dep : node.GetDirectDependencies())
        {
            consider(VersionFromImportName(dep.GetName()));
            if (const auto* p = dep.GetPath())
                consider(VersionFromResource(*p));
        }

        return result;
    }

    unordered_flat_map<string, VersionInfo> PeExecutable::GetMinimumDirectlyRequiredVersions() const
    {
        return CollectVersionsForNode(*this);
    }

    unordered_flat_map<string, VersionInfo> PeExecutable::GetMinimumRequiredVersions() const
    {
        auto maxByPrefix = unordered_flat_map<string, VersionInfo>();

        auto q = queue<const PeExecutable*>();
        q.push(this);

        while (!q.empty())
        {
            auto node = q.front();
            q.pop();

            for (auto& [prefix, ver] : CollectVersionsForNode(*node))
            {
                auto it = maxByPrefix.find(prefix);
                if (it == maxByPrefix.end() || ver.Parts > it->second.Parts)
                    maxByPrefix[prefix] = ver;
            }

            for (const auto& dep : node->GetDirectDependencies())
                q.push(&dep);
        }

        return maxByPrefix;
    }

    PeExecutable::PeExecutable(string name, optional<fs::path> path, vector<PeExecutable> directDependencies)
        : name(std::move(name)), path(std::move(path)), directDependencies(std::move(directDependencies))
    {
    }
} // namespace dyneeded
