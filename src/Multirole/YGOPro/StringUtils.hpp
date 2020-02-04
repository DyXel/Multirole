#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP
#include <string>

namespace Ignis
{

namespace StringUtils
{

std::u16string BufferToUTF16(const void* data, std::size_t maxCount);
std::size_t UTF16ToBuffer(void* data, std::u16string str);

std::string UTF16ToUTF8(std::u16string_view str);
std::u16string UTF8ToUTF16(std::string_view str);

} // namespace StringUtils

} // namespace Ignis

#endif // STRINGUTILS_HPP
