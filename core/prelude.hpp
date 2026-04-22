#pragma once
#include <boost/unordered/unordered_flat_map_fwd.hpp>
#include <boost/leaf.hpp>

namespace std
{
    namespace filesystem
    {
    }
} // namespace std

namespace dyneeded
{
    using namespace std;
    using namespace boost::leaf;
    namespace fs = ::std::filesystem;

    using boost::unordered_flat_map;
} // namespace dyneeded
