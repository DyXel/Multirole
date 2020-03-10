#ifndef YGOPRO_BANLIST_PARSER_HPP
#define YGOPRO_BANLIST_PARSER_HPP
#include <unordered_map>
#include "Banlist.hpp"

namespace YGOPro
{

using BanlistHash = uint32_t;

namespace Detail
{

constexpr const BanlistHash BANLIST_HASH_MAGIC = 0x7DFCEE6A;

constexpr BanlistHash Salt(BanlistHash hash, uint32_t code, uint32_t count)
{
	constexpr uint32_t HASH_MAGIC_1 = 18u;
	constexpr uint32_t HASH_MAGIC_2 = 14u;
	constexpr uint32_t HASH_MAGIC_3 = 27u;
	constexpr uint32_t HASH_MAGIC_4 = 5u;
	return hash ^ ((code << HASH_MAGIC_1) | (code >> HASH_MAGIC_2)) ^
	       ((code << (HASH_MAGIC_3 + count)) | (code >> (HASH_MAGIC_4 - count)));
}

} // namespace Detail

template<typename Stream>
std::unordered_map<BanlistHash, Banlist> ParseForBanlists(Stream& stream)
{
	std::unordered_map<BanlistHash, Banlist> banlists;
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
	while(std::getline(stream, l))
	{
		switch(l[0])
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
		case '#':
		case '$':
		{
			continue;
		}
		default:
		{
			std::size_t p = l.find(' ');
			if(p == std::string::npos)
				throw std::runtime_error("Card code separator not found");
			std::size_t c = l.find_first_not_of("0123456789", p + 1u);
			if(c != std::string::npos)
				c -= p;
			auto code = static_cast<uint32_t>(std::stol(l.substr(0u, p)));
			if(code == 0u)
				std::runtime_error("Card code cannot be 0");
			auto count = static_cast<uint32_t>(std::stol(l.substr(p, c)));
			hash = Detail::Salt(hash, code, count);
			switch(count)
			{
#define X(val, uset) case val: {uset.insert(code); break;}
			X(3u, whit)
			X(2u, semi)
			X(1u, limi)
			X(0u, forb)
#undef X
			default:
				throw std::runtime_error("Card count is not 0, 1, 2 or 3");
			}
			continue;
		}
		}
	}
	ConditionallyAdd();
	return banlists;
}

} // namespace YGOPro

#endif // YGOPRO_BANLIST_PARSER_HPP
