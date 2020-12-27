#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP
#include <string_view>

namespace FileSystem
{

// Creates a folder on the given path.
// Returns true if succesful or if directory already existed, false otherwise.
bool MakeDir(std::string_view path);

} // namespace FileSystem

#endif // FILESYSTEM_HPP
