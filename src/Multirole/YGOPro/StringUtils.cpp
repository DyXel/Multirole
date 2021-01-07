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


constexpr const char* ERROR_STR = "Invalid String";

#if defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_VER < 1920
using Wsc = std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t>;
#else
using Wsc = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>;
#endif // defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_VER < 1920

using WscElem = Wsc::wide_string::value_type;

inline Wsc MakeWsc()
{
	return Wsc{ERROR_STR};
}

std::string UTF16ToUTF8(std::u16string_view str)
{
	const auto* p = reinterpret_cast<const WscElem*>(str.data());
	return MakeWsc().to_bytes(p, p + str.size());
}

std::u16string UTF8ToUTF16(std::string_view str)
{
	if constexpr(std::is_same_v<char16_t, WscElem>)
		return MakeWsc().from_bytes(str.data());
	const auto r = MakeWsc().from_bytes(str.data(), str.data() + str.size());
	return std::u16string(reinterpret_cast<const char16_t*>(r.data()), r.length());
}

} // namespace YGOPro
