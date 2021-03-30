#ifndef MULTIROLE_UTILITY_HPP
#define MULTIROLE_UTILITY_HPP
#include <boost/filesystem/path.hpp>
#include <boost/json/string.hpp>

namespace Ignis::Multirole::Utility
{

inline boost::filesystem::path JStrToPath(const boost::json::string& str)
{
	return boost::filesystem::path(str.data());
}

} // namespace Ignis::Multirole::Utility

#endif // MULTIROLE_UTILITY_HPP

