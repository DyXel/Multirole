#ifndef ROOM_CONTEXT_HPP
#define ROOM_CONTEXT_HPP
#include <map>
#include <set>
#include <random>

#include "State.hpp"
#include "Event.hpp"
#include "../CoreProvider.hpp"
#include "../STOCMsgFactory.hpp"

namespace YGOPro
{

class Banlist;

} // namespace YGOPro

namespace Ignis::Multirole::Room
{

class Context : public STOCMsgFactory
{
public:
	Context(
		YGOPro::HostInfo&& hostInfo,
		YGOPro::DeckLimits&& limits,
		CoreProvider::CorePkg&& cpkg,
		const YGOPro::Banlist* banlist);

	const YGOPro::HostInfo& HostInfo() const;

	// Thread-safe getter for encoded information regarding duelists in the room
	std::map<uint8_t, std::string> GetDuelistsNames();

	void SetId(uint32_t newId);

	/*** STATE AND EVENT HANDLERS ***/
	// State/Waiting.cpp
	StateOpt operator()(State::Waiting& s, const Event::Join& e);
	StateOpt operator()(State::Waiting& s, const Event::ConnectionLost& e);
	StateOpt operator()(State::Waiting& s, const Event::ToObserver& e);
	StateOpt operator()(State::Waiting& s, const Event::ToDuelist& e);
	StateOpt operator()(State::Waiting&, const Event::Ready& e);
	StateOpt operator()(State::Waiting&, const Event::UpdateDeck& e);
	StateOpt operator()(State::Waiting&, const Event::TryKick& e);
	StateOpt operator()(State::Waiting& s, const Event::TryStart& e);
	// State/RockPaperScissor.cpp
	void operator()(State::RockPaperScissor&);
	StateOpt operator()(State::RockPaperScissor& s, const Event::ChooseRPS& e);
	// State/ChoosingTurn.cpp
	void operator()(State::ChoosingTurn& s);
	StateOpt operator()(State::ChoosingTurn& s, const Event::ChooseTurn& e);
	// State/Dueling.cpp
	void operator()(State::Dueling& s);
	StateOpt operator()(State::Dueling& s, const Event::Response& e);
	// State/Closing.cpp
	void operator()(State::Closing&);
	StateOpt operator()(State::Closing&, const Event::Join& e);
	StateOpt operator()(State::Closing&, const Event::ConnectionLost& e);

	// Chat handling is the same for all states
	template<typename State>
	constexpr StateOpt operator()(State&, const Event::Chat& e)
	{
		MakeAndSendChat(e.client, e.msg);
		return std::nullopt;
	}

	// Ignore rest of state entries
	template<typename S>
	constexpr void operator()(S&)
	{}

	// Ignore rest of state and event combinations
	template<typename S, typename E>
	constexpr StateOpt operator()(S&, E&)
	{
		return std::nullopt;
	}
private:
	const YGOPro::HostInfo hostInfo;
	const YGOPro::DeckLimits limits;
	CoreProvider::CorePkg cpkg;
	const YGOPro::Banlist* banlist;

	std::map<Client::PosType, Client*> duelists;
	std::mutex mDuelists;
	std::set<Client*> spectators;

	// Used by State/Dueling.cpp to generate duel seed and shuffle deck
	uint32_t id{};
	std::unique_ptr<std::mt19937> rng;

	// Utilities to send a message to multiple clients
	void SendToTeam(uint8_t team, const YGOPro::STOCMsg& msg);
	void SendToSpectators(const YGOPro::STOCMsg& msg);
	void SendToAll(const YGOPro::STOCMsg& msg);

	// Creates and sends to all a chat message from a client
	void MakeAndSendChat(Client& client, std::string_view msg);

	// Creates a YGOPro::Deck from the given vectors, making sure
	// that the deck is only composed of non-zero card codes
	std::unique_ptr<YGOPro::Deck> LoadDeck(
		const std::vector<uint32_t>& main,
		const std::vector<uint32_t>& side) const;

	// Check if a given deck is valid on the current room options
	std::unique_ptr<YGOPro::STOCMsg> CheckDeck(const YGOPro::Deck& deck) const;

	/*** STATE SPECIFIC FUNCTIONS ***/
	// State/Waiting.cpp
	bool TryEmplaceDuelist(Client& client, Client::PosType hint = {});
	// State/RockPaperScissor.cpp
	void SendRPS();
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_CONTEXT_HPP
