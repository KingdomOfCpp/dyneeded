#include "version_info.hpp"

#include <sstream>

namespace dyneeded
{
    result<VersionInfo> VersionInfo::Parse(string_view str)
    {
        // on elf format is like GLIBC_2.3.4
        auto underscore = str.find('_');
        if (underscore == string_view::npos)
            return new_error();

        auto prefix = str.substr(0, underscore);
        auto versionStr = str.substr(underscore + 1);
        auto ss = stringstream(string(versionStr));
        auto part = string();
        auto parts = vector<int>();

        while (getline(ss, part, '.'))
        {
            try
            {
                parts.push_back(stoi(part));
            }
            catch (...)
            {
                return new_error();
            }
        }

        return VersionInfo{.Prefix = string(prefix), .Parts = std::move(parts)};
    }
} // namespace dyneeded
