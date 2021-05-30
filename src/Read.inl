template<typename T>
constexpr T Read(const uint8_t*& ptr) noexcept
{
	T value{};
	std::memcpy(&value, ptr, sizeof(T));
	ptr += sizeof(T);
	return value;
}

template<typename T>
constexpr T Read(uint8_t*& ptr) noexcept
{
	return Read<T>(const_cast<const uint8_t*&>(ptr));
}
