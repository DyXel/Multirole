#ifndef ROOM_CONTEXT_HPP
#define ROOM_CONTEXT_HPP
#include <map>
#include <set>
#include <shared_mutex>
#include <random>

#include "Event.hpp"
#include "ScriptLogger.hpp"
#include "State.hpp"
#include "../Service.hpp"
#include "../STOCMsgFactory.hpp"

namespace YGOPro
{

class Banlist;
using BanlistPtr = std::shared_ptr<Banlist>;
class CardDatabase;

} // namespace YGOPro

namespace Ignis::Multirole
{

class RoomLogger;

namespace Room
{

class TimerAggregator;

class Context : public STOCMsgFactory
{
public:
	// Data passed on the ctor.
	struct CreateInfo
	{
		Service& svc;
		TimerAggregator& tagg;
		uint32_t id;
		uint32_t seed;
		YGOPro::BanlistPtr banlist;
		YGOPro::HostInfo hostInfo;
		YGOPro::DeckLimits limits;
		bool isPrivate;
		const std::string& notes;
	};

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
		uint8_t winner; // 2 == DRAW
	};

	Context(CreateInfo&& info);
	~Context();

	const YGOPro::HostInfo& HostInfo() const;
	bool IsPrivate() const;
	std::map<uint8_t, std::string> GetDuelistsNames() const;

	/*** STATE AND EVENT HANDLERS ***/
	// State/ChoosingTurn.cpp
	StateOpt operator()(State::ChoosingTurn& s);
	StateOpt operator()(State::ChoosingTurn& s, const Event::ChooseTurn& e);
	StateOpt operator()(State::ChoosingTurn&, const Event::ConnectionLost& e);
	StateOpt operator()(State::ChoosingTurn&, const Event::Join& e);
	// State/Closing.cpp
	StateOpt operator()(State::Closing&);
	StateOpt operator()(State::Closing&, const Event::Join& e);
	// State/Dueling.cpp
	StateOpt operator()(State::Dueling& s);
	StateOpt operator()(State::Dueling& s, const Event::ConnectionLost& e);
	StateOpt operator()(State::Dueling& s, const Event::Join& e);
	StateOpt operator()(State::Dueling& s, const Event::Response& e);
	StateOpt operator()(State::Dueling& s, const Event::Surrender& e);
	StateOpt operator()(State::Dueling& s, const Event::TimerExpired& e);
	// State/Rematching.cpp
	StateOpt operator()(State::Rematching&);
	StateOpt operator()(State::Rematching&, const Event::ConnectionLost& e);
	StateOpt operator()(State::Rematching& s, const Event::Rematch& e);
	StateOpt operator()(State::Rematching&, const Event::Join& e);
	// State/RockPaperScissor.cpp
	StateOpt operator()(State::RockPaperScissor&);
	StateOpt operator()(State::RockPaperScissor& s, const Event::ChooseRPS& e);
	StateOpt operator()(State::RockPaperScissor&, const Event::ConnectionLost& e);
	StateOpt operator()(State::RockPaperScissor&, const Event::Join& e);
	// State/Sidedecking.cpp
	StateOpt operator()(State::Sidedecking&);
	StateOpt operator()(State::Sidedecking&, const Event::ConnectionLost& e);
	StateOpt operator()(State::Sidedecking& s, const Event::UpdateDeck& e);
	StateOpt operator()(State::Sidedecking&, const Event::Join& e);
	// State/Waiting.cpp
	StateOpt operator()(State::Waiting&, const Event::Close&);
	StateOpt operator()(State::Waiting& s, const Event::ConnectionLost& e);
	StateOpt operator()(State::Waiting& s, const Event::Join& e);
	StateOpt operator()(State::Waiting& s, const Event::ToDuelist& e);
	StateOpt operator()(State::Waiting& s, const Event::ToObserver& e);
	StateOpt operator()(State::Waiting&, const Event::Ready& e);
	StateOpt operator()(State::Waiting& s, const Event::TryKick& e);
	StateOpt operator()(State::Waiting& s, const Event::TryStart& e);
	StateOpt operator()(State::Waiting&, const Event::UpdateDeck& e);

	// Chat handling is the same for all states.
	template<typename State>
	inline StateOpt operator()(State&, const Event::Chat& e)
	{
		MakeAndSendChat(e.client, e.msg);
		return std::nullopt;
	}

	// Ignore rest of state entries.
	template<typename S>
	inline StateOpt operator()(S&)
	{
		return std::nullopt;
	}

	// Ignore rest of state and event combinations.
	template<typename S, typename E>
	inline StateOpt operator()(S&, E&)
	{
		return std::nullopt;
	}
private:
	// Creation options and resources.
	Service& svc;
	TimerAggregator& tagg;
	const uint32_t id;
	const YGOPro::BanlistPtr banlist;
	const YGOPro::HostInfo hostInfo;
	const YGOPro::DeckLimits limits;
	const std::shared_ptr<YGOPro::CardDatabase> cdb;
	const int32_t neededWins;
	const YGOPro::STOCMsg joinMsg;
	const YGOPro::STOCMsg retryErrorMsg;
	const bool isPrivate;
	std::unique_ptr<RoomLogger> rl;
	ScriptLogger scriptLogger;

	// Client management variables.
	std::map<Client::PosType, Client*> duelists;
	mutable std::shared_mutex mDuelists;
	std::set<Client*> spectators;

	// Additional data used by room states.
	uint8_t isTeam1GoingFirst{};
	std::mt19937 rng{};
	std::array<int32_t, 2U> wins{};

	// Get correctly swapped teams based on team1 going first or not.
	uint8_t GetSwappedTeam(uint8_t team) const;

	// Get the number of duelists on each team.
	std::array<uint8_t, 2U> GetTeamCounts() const;

	// Utilities to send a message to multiple clients.
	void SendToTeam(uint8_t team, const YGOPro::STOCMsg& msg);
	void SendToSpectators(const YGOPro::STOCMsg& msg);
	void SendToAll(const YGOPro::STOCMsg& msg);
	void SendToAllExcept(Client& client, const YGOPro::STOCMsg& msg);

	// Creates the PlayerEnter and TypeChange messages for each duelist
	// and sends that information to the given client.
	void SendDuelistsInfo(Client& client);

	// Adds given client to the spectators set, sends the join message as
	// well as duelists information.
	void SetupAsSpectator(Client& client);

	// Creates and sends to all a chat message from a client.
	void MakeAndSendChat(Client& client, std::string_view msg);

	// Creates a YGOPro::Deck from the given vectors, making sure
	// that the deck is only composed of non-zero card codes, also,
	// sets its internal error to whatever was the lastest unknown card.
	std::unique_ptr<YGOPro::Deck> LoadDeck(
		const std::vector<uint32_t>& main,
		const std::vector<uint32_t>& side) const;

	// Check if a given deck is valid on the current room options.
	std::unique_ptr<YGOPro::STOCMsg> CheckDeck(const YGOPro::Deck& deck) const;

	/*** STATE SPECIFIC FUNCTIONS ***/
	// State/Dueling.cpp
	Client& GetCurrentTeamClient(State::Dueling& s, uint8_t team);
	std::optional<DuelFinishReason> Process(State::Dueling& s);
	StateVariant Finish(State::Dueling& s, const DuelFinishReason& dfr);
	static const YGOPro::STOCMsg& SaveToSpectatorCache(
		State::Dueling& s,
		YGOPro::STOCMsg&& msg);
	// State/RockPaperScissor.cpp
	void SendRPS();
	// State/Waiting.cpp
	bool TryEmplaceDuelist(Client& client, Client::PosType hint = {});
};

} // namespace Room

} // namespace Ignis::Multirole

#endif // ROOM_CONTEXT_HPP
