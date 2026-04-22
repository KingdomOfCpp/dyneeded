#pragma once

#include <boost/unordered/unordered_flat_map.hpp>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <glaze/glaze.hpp>
#include <vector>

#include "core/utils/version_info.hpp"

namespace LIEF::MachO
{
    class Binary;
}

namespace dyneeded
{
    class MachOExecutable
    {
        string name;
        optional<fs::path> path;
        vector<MachOExecutable> directDependencies;
        optional<string> version;
        vector<string> neededVersions; // raw version strings from LC_VERSION_MIN / BUILD_VERSION
    public:
        static result<MachOExecutable> FromBinary(const fs::path& file, const LIEF::MachO::Binary* macho);


        string_view GetName() const;
        const fs::path* GetPath() const;
        span<const MachOExecutable> GetDirectDependencies() const;
        vector<MachOExecutable> GetDependencies() const;
        optional<string_view> GetVersion() const;
        const vector<string>& GetNeededVersions() const;

        unordered_flat_map<string, VersionInfo> GetMinimumDirectlyRequiredVersions() const;
        unordered_flat_map<string, VersionInfo> GetMinimumRequiredVersions() const;

    private:
        MachOExecutable(string name, optional<fs::path> path,
                        vector<MachOExecutable> directDependencies,
                        optional<string> version,
                        vector<string> neededVersions);

        static vector<string> GetRpaths(const LIEF::MachO::Binary* binary, const fs::path& path);
        static optional<fs::path> FindLibrary(string_view name, span<const string> rpaths);
        static MachOExecutable AnalyzeMachOExecutable(const LIEF::MachO::Binary* binary,
                                                      string_view name,
                                                      span<const string> rpaths,
                                                      optional<fs::path> path,
                                                      unordered_set<string>& visited);

        static unordered_flat_map<string, VersionInfo>
        GetMinimumRequiredVersionsImpl(const MachOExecutable& executable);
    };
} // namespace dyneeded

template <>
struct glz::meta<dyneeded::MachOExecutable>
{
    using T = dyneeded::MachOExecutable;
    static constexpr auto value = glz::object(
        "name", [](const T& e) { return std::string(e.GetName()); },
        "path", [](const T& e) -> std::optional<std::string>
        {
            auto p = e.GetPath();
            return p ? std::optional<std::string>(p->string()) : std::nullopt;
        },
        "direct_dependencies", [](const T& e) { return e.GetDirectDependencies(); }
    );
};
