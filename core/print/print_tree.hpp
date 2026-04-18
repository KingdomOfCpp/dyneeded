#pragma once
#include "core/formats/elf_executable.hpp"

namespace dyneeded
{
void PrintElfTree(const ElfExecutable& exe, string prefix = "", bool isLast = true);
}