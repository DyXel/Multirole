#ifndef YGOPRO_BANLIST_PARSER_HPP
#define YGOPRO_BANLIST_PARSER_HPP
#include <unordered_map>

#include <fmt/format.h>

#include "Banlist.hpp"

namespace YGOPro
{

using BanlistHash = uint32_t;
using BanlistMap = std::unordered_map<BanlistHash, Banlist>;

namespace Detail
{

constexpr const BanlistHash BANLIST_HASH_MAGIC = 0x7DFCEE6A;

constexpr BanlistHash Salt(BanlistHash hash, uint32_t code, uint32_t count)
{
	constexpr uint32_t HASH_MAGIC_1 = 18U;
	constexpr uint32_t HASH_MAGIC_2 = 14U;
	constexpr uint32_t HASH_MAGIC_3 = 27U;
	constexpr uint32_t HASH_MAGIC_4 = 5U;
	return hash ^ ((code << HASH_MAGIC_1) | (code >> HASH_MAGIC_2)) ^
	       ((code << (HASH_MAGIC_3 + count)) | (code >> (HASH_MAGIC_4 - count)));
}

} // namespace Detail

template<typename Stream>
void ParseForBanlists(Stream& stream, BanlistMap& banlists)
{
	BanlistHash hash = Detail::BANLIST_HASH_MAGIC;
	CodeSet whit, semi, limi, forb;
	auto ConditionallyAdd = [&]()
	{
		if(hash == Detail::BANLIST_HASH_MAGIC)
			return;
		banlists.emplace(std::piecewise_construct, std::forward_as_tuple(hash),
			std::forward_as_tuple(
				std::move(whit),
				std::move(semi),
				std::move(limi),
				std::move(forb)
			)
		);
	};
	std::string l;
	std::size_t lc = 0U;
	auto MakeException = [&lc](std::string_view str)
	{
		return std::runtime_error(fmt::format("{:d}:{:s}", lc, str));
	};
	while(++lc, std::getline(stream, l))
	{
		switch(l[0U])
		{
		case '!':
		{
			ConditionallyAdd();
			// Reset state
			hash = Detail::BANLIST_HASH_MAGIC;
			whit.clear();
			semi.clear();
			limi.clear();
			forb.clear();
			continue;
		}
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			std::size_t p = l.find(' ');
			if(p == std::string::npos)
				throw MakeException("Card code separator not found");
			std::size_t c = l.find_first_not_of("0123456789", p + 1U);
			if(c != std::string::npos)
				c -= p;
			auto code = static_cast<uint32_t>(std::stoul(l.substr(0U, p)));
			if(code == 0U)
				throw MakeException("Card code cannot be 0");
			auto count = static_cast<uint32_t>(std::stoul(l.substr(p, c)));
			hash = Detail::Salt(hash, code, count);
			switch(count)
			{
#define X(val, uset) case val: {uset.insert(code); break;}
			X(3U, whit)
			X(2U, semi)
			X(1U, limi)
			X(0U, forb)
#undef X
			default:
				throw MakeException("Card count is not 0, 1, 2 or 3");
			}
			continue;
		}
		}
	}
	ConditionallyAdd();
}

} // namespace YGOPro

#endif // YGOPRO_BANLIST_PARSER_HPP
