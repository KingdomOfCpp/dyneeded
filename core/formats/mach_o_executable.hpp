#pragma once
#include <filesystem>
#include <LIEF/MachO/Binary.hpp>

#include "mach_o_executable.hpp"
#include "core/prelude.hpp"

namespace dyneeded
{
    class MachOExecutable
    {
    public:
        static result<MachOExecutable> FromBinary(const fs::path& file, const LIEF::MachO::Binary* macho);
    };
}