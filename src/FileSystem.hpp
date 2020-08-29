#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP
#include <string_view>

namespace FileSystem
{

bool DoesDirExist(std::string_view path);
bool MakeDir(std::string_view path);

bool DoesFileExist(std::string_view path);

} // namespace FileSystem

#endif // FILESYSTEM_HPP
