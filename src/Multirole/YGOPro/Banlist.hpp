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

	Banlist(bool whitelist, DictType dict);

	bool IsWhitelist() const;
	const DictType& Dict() const;
private:
	const bool whitelist;
	DictType dict;
};

using BanlistPtr = std::shared_ptr<Banlist>;

} // namespace Multirole

#endif // YGOPRO_BANLIST_HPP
