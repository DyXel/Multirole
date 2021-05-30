template<typename T>
constexpr void Write(uint8_t*& ptr, T value) noexcept
{
	std::memcpy(ptr, &value, sizeof(T));
	ptr += sizeof(T);
}
