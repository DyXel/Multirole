#include "StringUtils.hpp"

#include <cstring>
#include <codecvt>
#include <locale>

namespace YGOPro
{

std::u16string BufferToUTF16(const void* data, std::size_t maxByteCount)
{
	const auto* p = reinterpret_cast<const char16_t*>(data);
	const auto* p2 = p;
	std::size_t bytesForwarded = 0U;
	while(bytesForwarded <= maxByteCount && (*p != 0U))
	{
		p++;
		bytesForwarded += 2U;
	}
	return std::u16string(p2, p - p2);
}

#if defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_VER < 1920
#define WCI std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t>{}
std::string UTF16ToUTF8(std::u16string_view str)
{
	auto p = reinterpret_cast<const int16_t*>(str.data());
	return WCI.to_bytes(p, p + str.size());
}

std::u16string UTF8ToUTF16(std::string_view str)
{
	auto asInt = WCI.from_bytes(str.data(), str.data() + str.size());
	return std::u16string(reinterpret_cast<char16_t const*>(asInt.data()), asInt.length());
}
#else
#define WCI std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
std::string UTF16ToUTF8(std::u16string_view str)
{
	return WCI.to_bytes(str.data());
}

std::u16string UTF8ToUTF16(std::string_view str)
{
	return WCI.from_bytes(str.data());
}
#endif
#undef WCI

} // namespace YGOPro
