#include "StringUtils.hpp"

#include <cstring>
#include <codecvt>
#include <locale>

namespace YGOPro
{

std::u16string BufferToUTF16(const void* data, std::size_t maxByteCount)
{
	auto p = reinterpret_cast<const char16_t*>(data), p2 = p;
	std::size_t bytesForwarded = 0u;
	while(bytesForwarded <= maxByteCount && *p)
	{
		p++;
		bytesForwarded += 2u;
	}
	return std::u16string(p2, p - p2);
}

std::size_t UTF16ToBuffer(void* data, std::u16string str)
{
	std::size_t bytesCopied = (str.size() + 1u) * sizeof(char16_t);
	std::memcpy(data, str.data(), bytesCopied);
	return bytesCopied;
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

} // namespace YGOPro
