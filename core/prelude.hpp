#pragma once
#include "boost/unordered/unordered_flat_map_fwd.hpp"
#include <boost/leaf/error.hpp>
#include <boost/leaf/result.hpp>

namespace std
{
namespace filesystem
{
}
} // namespace std

namespace dyneeded
{
using namespace std;
namespace fs = ::std::filesystem;

using boost::unordered_flat_map;
using boost::leaf::new_error;
using boost::leaf::result;
} // namespace dyneeded
