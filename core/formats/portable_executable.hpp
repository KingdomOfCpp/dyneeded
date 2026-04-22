#pragma once
#include "../prelude.hpp"
#include "core/utils/version_info.hpp"

#include <glaze/glaze.hpp>
#include <filesystem>
#include <unordered_set>

namespace LIEF::PE
{
    class Binary;
}

namespace dyneeded
{
    struct FailedToParsePeError
    {
    };

    class PeExecutable
    {
        string name;
        optional<fs::path> path;
        vector<PeExecutable> directDependencies;

    public:
        /**
         * Analyse a PE executable (.exe or .dll)
         * @param file Path of the PE file
         * @param pe   Already-parsed LIEF PE binary (borrowed, must outlive this call)
         * @return The analysis result as PeExecutable or FailedToParsePeError
         */
        static result<PeExecutable> FromBinary(const fs::path& file, const LIEF::PE::Binary* pe);

        string_view GetName() const;
        const fs::path* GetPath() const;
        span<const PeExecutable> GetDirectDependencies() const;
        vector<PeExecutable> GetDependencies() const;
        unordered_flat_map<string, VersionInfo> GetMinimumRequiredVersions() const;
        unordered_flat_map<string, VersionInfo> GetMinimumDirectlyRequiredVersions() const;

    private:
        PeExecutable(string name, optional<fs::path> path, vector<PeExecutable> directDependencies);

        static PeExecutable AnalyzePeExecutable(const LIEF::PE::Binary* binary, string_view name,
                                                const fs::path& searchRoot, optional<fs::path> path,
                                                unordered_set<string>& visited);

        static optional<fs::path> FindLibrary(string_view name, const fs::path& searchRoot);
    };
} // namespace dyneeded

template <>
struct glz::meta<dyneeded::PeExecutable>
{
    using T = dyneeded::PeExecutable;
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
