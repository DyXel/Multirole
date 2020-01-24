#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP
#include <string>

namespace Ignis
{

namespace StringUtils
{

std::string UTF16ToUTF8(std::u16string_view str);
std::u16string UTF8ToUTF16(std::string_view str);

std::u16string BufferToUTF16(void* data, std::size_t maxCount);

} // namespace StringUtils

} // namespace Ignis

#endif // STRINGUTILS_HPP
