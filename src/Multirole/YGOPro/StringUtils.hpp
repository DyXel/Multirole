#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP
#include <string>

namespace YGOPro
{

constexpr std::size_t UTF16ByteCount(std::u16string_view str) noexcept
{
	return (str.size() + 1U) * sizeof(char16_t);
}

std::u16string BufferToUTF16(const void* data, std::size_t maxByteCount) noexcept;

std::string UTF16ToUTF8(std::u16string_view str) noexcept;
std::u16string UTF8ToUTF16(std::string_view str) noexcept;

} // namespace YGOPro

#endif // STRINGUTILS_HPP
