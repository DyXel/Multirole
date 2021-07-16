#ifndef YGOPRO_BANLIST_HPP
#define YGOPRO_BANLIST_HPP
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace YGOPro
{

class Banlist final
{
public:
	using DictType = std::unordered_map<uint32_t /*code*/, int32_t /*count*/>;

	Banlist(bool whitelist, DictType dict) noexcept;

	bool IsWhitelist() const noexcept;
	const DictType& Dict() const noexcept;
private:
	const bool whitelist;
	DictType dict;
};

using BanlistPtr = std::shared_ptr<Banlist>;

} // namespace Multirole

#endif // YGOPRO_BANLIST_HPP
