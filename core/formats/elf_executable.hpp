#pragma once
#include "../prelude.hpp"
#include <filesystem>
#include <unordered_set>

namespace LIEF::ELF
{
class Binary;
}

namespace dyneeded
{
struct FailedToParseElfError {};

class ElfExecutable
{
    string name;
    optional<fs::path> path;
    vector<ElfExecutable> dynamicDependencies;
    vector<string> neededVersions;
    optional<string> version;

  public:
    /**
     * Analyse an elf executable
     * @param file Path of the elf file
     * @return The analysis resus as ElfExecutable or FailedToParseElfError
     */
    static result<ElfExecutable> FromFile(const fs::path &file);

    string_view GetName() const;
    const fs::path *GetPath() const;
    span<const ElfExecutable> GetDependencies() const;
    optional<string_view> GetVersion() const;
    const vector<string>& GetNeededVersions() const;

  private:
    ElfExecutable(string name, optional<fs::path> path, vector<ElfExecutable> dynamic_dependencies,
                  optional<string> version, vector<string> neededVersions);

    static ElfExecutable AnalyzeElfExecutable(const LIEF::ELF::Binary *binary, string_view name,
                                              span<const string> rpaths, optional<fs::path> path,
                                              unordered_set<string> &visited);
};
} // namespace dyneeded
