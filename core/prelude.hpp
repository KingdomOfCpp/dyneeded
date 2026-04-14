#pragma once
#include <boost/leaf/result.hpp>
#include <boost/leaf/error.hpp>
#include "boost/unordered/unordered_flat_map_fwd.hpp"

namespace std
{
    namespace filesystem
    {
    }
}

namespace dyneeded
{
    using namespace std;
    namespace fs = ::std::filesystem;

    using boost::leaf::result;
    using boost::leaf::new_error;
    using boost::unordered_flat_map;
}
