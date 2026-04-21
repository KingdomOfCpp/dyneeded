#include "mach_o_executable.hpp"

namespace dyneeded
{
    result<MachOExecutable> MachOExecutable::FromBinary(const fs::path& file, const LIEF::MachO::Binary* macho)
    {
        return new_error();
    }
} // dyneeded
