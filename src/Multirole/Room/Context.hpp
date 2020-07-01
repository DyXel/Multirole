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
	StateOpt operator()(State::RockPaperScissor&);
	StateOpt operator()(State::RockPaperScissor&, const Event::ConnectionLost& e);
	StateOpt operator()(State::RockPaperScissor& s, const Event::ChooseRPS& e);
	// State/ChoosingTurn.cpp
	StateOpt operator()(State::ChoosingTurn& s);
	StateOpt operator()(State::ChoosingTurn&, const Event::ConnectionLost& e);
	StateOpt operator()(State::ChoosingTurn& s, const Event::ChooseTurn& e);
	// State/Dueling.cpp
	StateOpt operator()(State::Dueling& s);
	StateOpt operator()(State::Dueling& s, const Event::ConnectionLost& e);
	StateOpt operator()(State::Dueling& s, const Event::Response& e);
	StateOpt operator()(State::Dueling& s, const Event::Surrender& e);
	// State/Closing.cpp
	StateOpt operator()(State::Closing&);
	StateOpt operator()(State::Closing&, const Event::Join& e);
	StateOpt operator()(State::Closing&, const Event::ConnectionLost& e);
	// State/Rematching.cpp
	StateOpt operator()(State::Rematching& s);
	StateOpt operator()(State::Rematching&, const Event::ConnectionLost& e);
	StateOpt operator()(State::Rematching& s, const Event::Rematch& e);

	// Chat handling is the same for all states
	template<typename State>
	constexpr StateOpt operator()(State&, const Event::Chat& e)
	{
		MakeAndSendChat(e.client, e.msg);
		return std::nullopt;
	}

	// Ignore rest of state entries
	template<typename S>
	constexpr StateOpt operator()(S&)
	{
		return std::nullopt;
	}

	// Ignore rest of state and event combinations
	template<typename S, typename E>
	constexpr StateOpt operator()(S&, E&)
	{
		return std::nullopt;
	}
private:
	struct DuelFinishReason
	{
		enum class Reason : uint8_t
		{
			REASON_DUEL_WON,
			REASON_SURRENDERED,
			REASON_TIMED_OUT,
			REASON_WRONG_RESPONSE,
			REASON_CONNECTION_LOST,
			REASON_CORE_CRASHED,
		} reason;
		uint8_t team;
	};

	// Creation options and resources
	const YGOPro::HostInfo hostInfo;
	const YGOPro::DeckLimits limits;
	CoreProvider::CorePkg cpkg;
	const YGOPro::Banlist* banlist;
	const int32_t neededWins;

	// Client management variables
	std::map<Client::PosType, Client*> duelists;
	std::array<uint8_t, 2> teamCount;
	std::mutex mDuelists;
	std::set<Client*> spectators;

	// Additional data used by room states
	uint8_t isTeam1GoingFirst{};
	uint32_t id{};
	std::unique_ptr<std::mt19937> rng;
	std::array<int32_t, 2> wins{};

	// Get correctly swapped teams based on team1 going first or not
	uint8_t GetSwappedTeam(uint8_t team);

	// Utilities to send a message to multiple clients
	void SendToTeam(uint8_t team, const YGOPro::STOCMsg& msg);
	void SendToSpectators(const YGOPro::STOCMsg& msg);
	void SendToAll(const YGOPro::STOCMsg& msg);
	void SendToAllExcept(Client& client, const YGOPro::STOCMsg& msg);

	// Creates and sends to all a chat message from a client
	void MakeAndSendChat(Client& client, std::string_view msg);

	// Creates a YGOPro::Deck from the given vectors, making sure
	// that the deck is only composed of non-zero card codes, also,
	// sets its internal error to whatever was the lastest unknown card.
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
	// State/Dueling.cpp
	Client& GetCurrentTeamClient(State::Dueling& s, uint8_t team);
	std::optional<DuelFinishReason> Process(State::Dueling& s);
	StateVariant Finish(State::Dueling& s, const DuelFinishReason& dfr);
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_CONTEXT_HPP
