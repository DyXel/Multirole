#include "../Context.hpp"

#include "../../Core/IScriptSupplier.hpp"

namespace Ignis::Multirole::Room
{

void Context::operator()(State::Dueling& s)
{
	using namespace YGOPro;
	// RNG is conditionally created here because most rooms would have not
	// reached this point and allocating it when the room is created
	// would have been a waste.
	if(!rng)
		rng = std::make_unique<std::mt19937>(id);
	// TODO: Set up Replay
	// Create core duel with room's options
	auto& core = *cpkg.core;
	const OCG_Player popt =
	{
		static_cast<int>(hostInfo.startingLP),
		hostInfo.startingDrawCount,
		hostInfo.drawCountPerTurn
	};
	const Core::IHighLevelWrapper::DuelOptions dopts =
	{
		static_cast<uint32_t>((*rng)()),
		static_cast<int>(hostInfo.duelFlags),
		popt,
		popt
	};
	try
	{
		s.duelPtr = core.CreateDuel(dopts);
		auto& supplier = *core.GetScriptSupplier();
		auto LoadScript = [&](std::string_view file)
		{
			if(auto scr = supplier.ScriptFromFilePath(file); !scr.empty())
				core.LoadScript(s.duelPtr, file, scr);
		};
		LoadScript("constant.lua");
		LoadScript("utility.lua");
	}
	catch(...) { throw; } // TODO
	// Extra rules are controlled simply by adding custom cards
	// with the rulesets to the game as playable cards, they will
	// trigger immediately after the duel starts.
	std::vector<uint32_t> extraCards;
#define X(f, c) if(hostInfo.extraRules & (f)) extraCards.push_back(c)
	// NOTE: no lint used because we dont want clang-tidy to complain
	// about magic numbers we know are not going to change soon.
	X(EXTRA_RULE_SEALED_DUEL,        511005092); // NOLINT
	X(EXTRA_RULE_BOOSTER_DUEL,       511005093); // NOLINT
	X(EXTRA_RULE_DESTINY_DRAW,       511004000); // NOLINT
	X(EXTRA_RULE_CONCENTRATION_DUEL, 511004322); // NOLINT
	X(EXTRA_RULE_BOSS_DUEL,          95000000);  // NOLINT
	X(EXTRA_RULE_BATTLE_CITY,        511004014); // NOLINT
	X(EXTRA_RULE_DUELIST_KINGDOM,    511002621); // NOLINT
	X(EXTRA_RULE_DIMENSION_DUEL,     511600002); // NOLINT
	X(EXTRA_RULE_TURBO_DUEL,         110000000); // NOLINT
	X(EXTRA_RULE_COMMAND_DUEL,       95200000);  // NOLINT
	X(EXTRA_RULE_DECK_MASTER,        300);       // NOLINT
	X(EXTRA_RULE_ACTION_DUEL,        151999999); // NOLINT
#undef X
	OCG_NewCardInfo nci{};
	try
	{
		nci.pos = 0x8; // POS_FACEDOWN_DEFENSE
		for(auto code : extraCards)
		{
			nci.code = code;
			core.AddCard(s.duelPtr, nci);
		}
	}
	catch(...) { throw; } // TODO
	// Add main and extra deck cards for all players
	auto ReversedOrShuffled = [&](CodeVector deck) // NOTE: Copy is intentional.
	{
		if(hostInfo.dontShuffleDeck != 0u)
			std::reverse(deck.begin(), deck.end());
		else
			std::shuffle(deck.begin(), deck.end(), *rng);
		return deck;
	};
	try
	{
		for(const auto& kv : duelists)
		{
			auto& deck = *kv.second->CurrentDeck();
			nci.team = nci.con = kv.first.first;
			nci.duelist = kv.first.second;
			nci.loc = 0x01; // LOCATION_DECK
			for(auto code : ReversedOrShuffled(deck.Main()))
			{
				nci.code = code;
				core.AddCard(s.duelPtr, nci);
			}
			nci.loc = 0x40; // LOCATION_EXTRA
			for(auto code : deck.Extra())
			{
				nci.code = code;
				core.AddCard(s.duelPtr, nci);
			}
		}
	}
	catch(...) { throw; } // TODO
	// Send MSG_START message to clients
	using u8 = uint8_t;
	using u16 = uint16_t;
	using u32 = uint32_t;
	std::vector<u8> msg;
	msg.reserve(18);
	auto W = [&msg](const auto& value)
	{
		auto sz = msg.size();
		msg.resize(sz + sizeof(decltype(value)));
		std::memcpy(&msg[sz], &value, sizeof(decltype(value)));
	};
#define SC static_cast
	W(SC<u8>(0x04)); // MSG_START
	W(SC<u8>(0x00)); // For team 0
	W(SC<u32>(hostInfo.startingLP));
	W(SC<u32>(hostInfo.startingLP));
	try
	{
#define QC(t, l) core.QueryCount(s.duelPtr, t, l)
		W(SC<u16>(QC(0, 0x01))); // LOCATION_DECK
		W(SC<u16>(QC(0, 0x40))); // LOCATION_EXTRA
		W(SC<u16>(QC(1, 0x01))); // LOCATION_DECK
		W(SC<u16>(QC(1, 0x40))); // LOCATION_EXTRA
#undef QC
	}
	catch(...) { throw; } // TODO
#undef SC
	SendToTeam(0, STOCMsg{STOCMsg::MsgType::GAME_MSG, msg});
	msg[1] = 1; // For team 1 now.
	SendToTeam(1, STOCMsg{STOCMsg::MsgType::GAME_MSG, msg});
}

StateOpt Context::operator()(State::Dueling& s, const Event::Response& e)
{
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
