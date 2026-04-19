#pragma once
#include <filesystem>
#include <LIEF/span.hpp>
#include <LIEF/PE/Binary.hpp>

#include "core/prelude.hpp"

namespace dyneeded
{

class PortableExecutable
{
public:
    static result<PortableExecutable> FromBinary(const LIEF::PE::Binary* binary, const fs::path& path);
};

} // namespace dyneeded
