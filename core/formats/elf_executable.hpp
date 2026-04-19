#pragma once
#include "../prelude.hpp"
#include "core/utils/version_info.hpp"

#include <glaze/glaze.hpp>
#include <filesystem>
#include <unordered_set>

namespace LIEF::ELF
{
    class Binary;
}

namespace dyneeded
{
    struct FailedToParseElfError
    {
    };

    class ElfExecutable
    {
        string name;
        optional<fs::path> path;
        vector<ElfExecutable> directDependencies;
        vector<string> neededVersions;
        optional<string> version;

    public:
        /**
         * Analyse an elf executable
         * @param file Path of the elf file
         * @return The analysis resus as ElfExecutable or FailedToParseElfError
         */
        static result<ElfExecutable> FromBinary(const fs::path& file, const LIEF::ELF::Binary* elf);

        unordered_flat_map<string, VersionInfo> GetMinimumRequiredVersions() const;

        string_view GetName() const;
        const fs::path* GetPath() const;
        span<const ElfExecutable> GetDirectDependencies() const;
        vector<ElfExecutable> GetDependencies() const;
        optional<string_view> GetVersion() const;
        const vector<string>& GetNeededVersions() const;

    private:
        ElfExecutable(string name, optional<fs::path> path, vector<ElfExecutable> dynamic_dependencies,
                      optional<string> version, vector<string> neededVersions);

        static ElfExecutable AnalyzeElfExecutable(const LIEF::ELF::Binary* binary, string_view name,
                                                  span<const string> rpaths, optional<fs::path> path,
                                                  unordered_set<string>& visited);

        static unordered_flat_map<string, VersionInfo> GetMinimumRequiredVersionsImpl(const ElfExecutable& executable);
    };
} // namespace dyneeded

// we need a meta struct for glaze to work in this case
template <>
struct glz::meta<dyneeded::ElfExecutable>
{
    using T = dyneeded::ElfExecutable;
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
