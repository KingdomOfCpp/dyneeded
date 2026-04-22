#pragma once
#include <filesystem>

#include "core/prelude.hpp"

namespace dyneeded
{
    enum class ExeFormat
    {
        Elf,
        MachO,
        Portable,
    };
}
