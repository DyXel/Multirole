#include "StringUtils.hpp"

#include <codecvt>
#include <locale>

namespace Ignis
{

namespace StringUtils
{

std::u16string BufferToUTF16(void* data, std::size_t maxCount)
{
	auto p = reinterpret_cast<char16_t*>(data), p2 = p;
	std::size_t ntPos = 0u; // Null terminator position.
	while(ntPos < maxCount && *p != 0u)
	{
		p++;
		ntPos++;
	}
	return std::u16string(p2, ntPos);
}


#define WCI std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
std::string UTF16ToUTF8(std::u16string_view str)
{
	return WCI.to_bytes(str.data());
}

std::u16string UTF8ToUTF16(std::string_view str)
{
	return WCI.from_bytes(str.data());
}
#undef WCI

} // namespace StringUtils

} // namespace Ignis

