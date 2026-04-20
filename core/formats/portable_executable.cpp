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

optional<VersionInfo> VersionFromResource(const fs::path &dllPath)
{
    auto pe = LIEF::PE::Parser::parse(dllPath.string());
    if (!pe)
        return nullopt;

    auto mgr_result = pe->resources_manager();
    if (!mgr_result)
        return nullopt; // result<T>, not a pointer

    const auto &mgr = *mgr_result;
    if (!mgr.has_version())
        return nullopt;

    const auto versions = mgr.version(); // std::vector<ResourceVersion>
    if (versions.empty())
        return nullopt;

    // Take the first VS_VERSIONINFO entry (virtually always exactly one)
    const auto &ffi = versions[0].file_info(); // const fixed_file_info_t&

    // Validate the magic — malformed resources can have signature == 0
    if (ffi.signature != LIEF::PE::ResourceVersion::fixed_file_info_t::SIGNATURE_VALUE)
        return nullopt;

    // file_version_ms: high word = major, low word = minor
    // file_version_ls: high word = patch, low word = build
    const uint16_t major = static_cast<uint16_t>(ffi.file_version_ms >> 16);
    const uint16_t minor = static_cast<uint16_t>(ffi.file_version_ms & 0xFFFF);
    const uint16_t patch = static_cast<uint16_t>(ffi.file_version_ls >> 16);

    // Use the DLL stem as the prefix, e.g. "VCRUNTIME140" -> "VCRUNTIME140-14.0.9"
    auto stem = dllPath.stem().string();
    for (auto &c : stem)
        c = static_cast<char>(toupper(c));

    return VersionInfo{stem, {major, minor, patch}};
}

optional<VersionInfo> VersionFromImportName(string_view rawName)
{
    // Work with an uppercased, extension-stripped copy
    auto name = string(rawName);
    for (auto &c : name)
        c = static_cast<char>(toupper(c));
    if (auto dot = name.rfind('.'); dot != string::npos)
        name.erase(dot);

    // --- Pattern 1: api-ms-win-crt-* / api-ms-win-* (ApiSet DLLs) --------
    // Format: api-ms-win-<subsystem>-l<layer>-<major>-<minor>
    // e.g.    api-ms-win-crt-runtime-l1-1-0
    {
        static const std::regex kApiSet(R"(^(API-MS-WIN-[A-Z0-9-]+-L\d+)-(\d+)-(\d+)$)", std::regex::icase);
        std::smatch m;
        if (std::regex_match(name, m, kApiSet))
            return VersionInfo::Parse(fmt::format("{}-{}.{}", m[1].str(), m[2].str(), m[3].str())).value();
    }

    // --- Pattern 2: classic MSVC CRT / STL DLLs --------------------------
    // Format: <PREFIX><version>[_<suffix>]
    // e.g.    MSVCR140, MSVCP140_1, VCRUNTIME140_1
    {
        static const std::regex kMsvc(R"(^(MSVCR|MSVCP|VCRUNTIME|CONCRT|VCCORLIB|MSVCM)(\d+)(?:_(\d+))?$)");
        std::smatch m;
        if (std::regex_match(name, m, kMsvc))
        {
            auto vstr = m[3].matched ? fmt::format("{}-{}.{}", m[1].str(), m[2].str(), m[3].str())
                                     : fmt::format("{}-{}", m[1].str(), m[2].str());
            return VersionInfo::Parse(vstr).value();
        }
    }

    return nullopt;
}

optional<fs::path> PeExecutable::FindLibrary(string_view name, const fs::path &searchRoot)
{
#ifdef DYNEEDED_WINDOWS
    // SearchPath replicates the real Windows loader order:
    // 1. Application directory (lpPath override)
    // 2. System32
    // 3. 16-bit system directory
    // 4. Windows directory
    // 5. CWD
    // 6. PATH
    //
    // We call it twice: first with the binary's own directory prepended
    // (matching LOAD_WITH_ALTERED_SEARCH_PATH behaviour), then without.

    auto nameStr = string(name);

    // First pass: searchRoot prepended
    {
        TCHAR result[MAX_PATH];
        if (SearchPathA(searchRoot.string().c_str(), nameStr.c_str(), nullptr, MAX_PATH, result, nullptr) > 0)
            return fs::path(result);
    }

    // Second pass: standard order (SearchPath with lpPath = NULL)
    {
        TCHAR result[MAX_PATH];
        if (SearchPathA(nullptr, nameStr.c_str(), nullptr, MAX_PATH, result, nullptr) > 0)
            return fs::path(result);
    }

    return nullopt;

#else
    // Cross-platform fallback for analysing PE files on Linux/macOS
    if (auto p = searchRoot / name; fs::exists(p))
        return p;

    if (const char *env = getenv("PATH"))
    {
        auto ss = stringstream(env);
        string token;
        while (getline(ss, token, ':'))
            if (auto p = fs::path(token) / name; fs::exists(p))
                return p;
    }

    for (const auto &dir : kSearchPaths)
        if (auto p = dir / name; fs::exists(p))
            return p;

    return nullopt;
#endif
}

// ---------------------------------------------------------------------------
// AnalyzePeExecutable  (recursive)
// ---------------------------------------------------------------------------
PeExecutable PeExecutable::AnalyzePeExecutable(const LIEF::PE::Binary *binary, string_view name,
                                               const fs::path &searchRoot, optional<fs::path> path,
                                               unordered_set<string> &visited)
{
    auto deps = vector<PeExecutable>();

    for (const auto &imp : binary->imports())
    {
        const string &dllName = imp.name();
        if (dllName.empty())
            continue;

        auto foundLib = FindLibrary(dllName, searchRoot);
        if (!foundLib)
        {
            // Record as an unresolved dependency (no path)
            deps.push_back(PeExecutable(dllName, nullopt, {}));
            continue;
        }

        // Canonicalise to deduplicate symlinks / relative paths
        std::error_code ec;
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
        deps.push_back(AnalyzePeExecutable(child.get(), foundLib->filename().string(), childRoot, *foundLib, visited));
    }

    return {string(name), std::move(path), std::move(deps)};
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
result<PeExecutable> PeExecutable::FromBinary(const fs::path &file, const LIEF::PE::Binary *pe)
{
    std::error_code ec;
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
const fs::path *PeExecutable::GetPath() const
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
    auto q = queue<const PeExecutable *>();

    for (const auto &dep : directDependencies)
        q.push(&dep);

    while (!q.empty())
    {
        auto node = q.front();
        q.pop();

        auto key = node->path ? fs::weakly_canonical(*node->path).string() : string(node->GetName());

        if (!visited.insert(key).second)
            continue;

        result.push_back(*node);

        for (const auto &dep : node->directDependencies)
            q.push(&dep);
    }

    return result;
}

// Collect the max version per prefix across this node's import list,
// optionally enriched by on-disk resource data when a path is available.
static unordered_flat_map<string, VersionInfo> CollectVersionsForNode(const PeExecutable &node)
{
    auto result = unordered_flat_map<string, VersionInfo>();

    auto consider = [&](optional<VersionInfo> parsed) {
        if (!parsed)
            return;
        auto it = result.find(parsed->Prefix);
        if (it == result.end() || parsed->Parts > it->second.Parts)
            result[parsed->Prefix] = *parsed;
    };

    for (const auto &dep : node.GetDirectDependencies())
    {
        // Primary: name-based heuristic (always available)
        consider(VersionFromImportName(dep.GetName()));

        // Secondary: version resource of the resolved DLL on disk (richer data)
        if (const auto *p = dep.GetPath())
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

    auto q = queue<const PeExecutable *>();
    q.push(this);

    while (!q.empty())
    {
        auto node = q.front();
        q.pop();

        for (auto &[prefix, ver] : CollectVersionsForNode(*node))
        {
            auto it = maxByPrefix.find(prefix);
            if (it == maxByPrefix.end() || ver.Parts > it->second.Parts)
                maxByPrefix[prefix] = ver;
        }

        for (const auto &dep : node->GetDirectDependencies())
            q.push(&dep);
    }

    return maxByPrefix;
}

PeExecutable::PeExecutable(string name, optional<fs::path> path, vector<PeExecutable> directDependencies)
    : name(std::move(name)), path(std::move(path)), directDependencies(std::move(directDependencies))
{
}
} // namespace dyneeded