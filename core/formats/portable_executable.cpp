#include "portable_executable.hpp"

namespace dyneeded
{
    result<PortableExecutable> PortableExecutable::FromBinary(const LIEF::PE::Binary* binary, const fs::path& path)
    {
        return new_error();
    }
} // namespace dyneeded