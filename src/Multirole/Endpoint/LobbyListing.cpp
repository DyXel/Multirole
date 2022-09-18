#include "LobbyListing.hpp"

#include <boost/asio/write.hpp>
#include <boost/json.hpp>
#include <fmt/format.h> // fmt::to_string

#include "../Lobby.hpp"
#include "../Workaround.hpp"

namespace Ignis::Multirole::Endpoint
{

class LobbyListing::Connection final : public std::enable_shared_from_this<Connection>
{
public:
	Connection(
		boost::asio::ip::tcp::socket socket,
		std::shared_ptr<const std::string> data) noexcept
		:
		socket(std::move(socket)),
		outgoing(std::move(data)),
		incoming(),
		writeCalled(false)
	{}

	void DoRead() noexcept
	{
		auto self(shared_from_this());
		socket.async_read_some(boost::asio::buffer(incoming),
		[this, self](boost::system::error_code ec, std::size_t /*unused*/)
		{
			if(ec)
				return;
			if(!writeCalled)
			{
				writeCalled = true;
				DoWrite();
			}
			DoRead();
		});
	}
private:
	boost::asio::ip::tcp::socket socket;
	std::shared_ptr<const std::string> outgoing;
	std::array<char, 256U> incoming;
	bool writeCalled;

	void DoWrite() noexcept
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket, boost::asio::buffer(*outgoing),
		[this, self](boost::system::error_code ec, std::size_t /*unused*/)
		{
			if(!ec)
				socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		});
	}
};

// public

LobbyListing::LobbyListing(
	boost::asio::io_context& ioCtx,
	unsigned short port,
	Lobby& lobby)
	:
	acceptor(ioCtx, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port)),
	serializeTimer(ioCtx),
	lobby(lobby),
	serialized(std::make_shared<std::string>())
{
	Workaround::SetCloseOnExec(acceptor.native_handle());
	acceptor.set_option(boost::asio::socket_base::keep_alive(true));
	DoAccept();
	DoSerialize();
}

LobbyListing::~LobbyListing() = default;

void LobbyListing::Stop()
{
	acceptor.close();
	serializeTimer.cancel();
}

// private

void LobbyListing::DoSerialize()
{
	serializeTimer.expires_after(std::chrono::seconds(2));
	serializeTimer.async_wait([this](boost::system::error_code ec)
	{
		if(ec)
			return;
		boost::json::monotonic_resource mr;
		boost::json::object j(&mr);
		auto& ar = *j.emplace("rooms", boost::json::array(&mr)).first->value().if_array();
		lobby.CollectRooms([&](const Lobby::RoomProps& rp)
		{
			const auto& hi = *rp.hostInfo;
			auto& room = *ar.emplace_back(boost::json::object(21U, &mr)).if_object();
			room.emplace("roomid", rp.id);
			room.emplace("roomname", ""); // NOTE: UNUSED but expected atm
			room.emplace("roomnotes", *rp.notes);
			room.emplace("roommode", 0); // NOTE: UNUSED but expected atm
			room.emplace("needpass", rp.passworded);
			room.emplace("team1", hi.t0Count);
			room.emplace("team2", hi.t1Count);
			room.emplace("best_of", hi.bestOf);
			room.emplace("duel_flag", YGOPro::HostInfo::OrDuelFlags(hi.duelFlagsHigh, hi.duelFlagsLow));
			room.emplace("forbidden_types", hi.forb);
			room.emplace("extra_rules", hi.extraRules);
			room.emplace("start_lp", hi.startingLP);
			room.emplace("start_hand", hi.startingDrawCount);
			room.emplace("draw_count", hi.drawCountPerTurn);
			room.emplace("time_limit", hi.timeLimitInSeconds);
			room.emplace("rule", hi.allowed);
			room.emplace("no_check", static_cast<bool>(hi.dontCheckDeckContent));
			room.emplace("no_shuffle", static_cast<bool>(hi.dontShuffleDeck));
			room.emplace("banlist_hash", hi.banlistHash);
			room.emplace("istart", rp.started ? "start" : "waiting");
			room.emplace("main_min", hi.limits.main.min);
			room.emplace("main_max", hi.limits.main.max);
			room.emplace("extra_min", hi.limits.extra.min);
			room.emplace("extra_max", hi.limits.extra.max);
			room.emplace("side_min", hi.limits.side.min);
			room.emplace("side_max", hi.limits.side.max);
			const auto dCount = rp.duelists.usedCount;
			auto& ac = *room.emplace("users", boost::json::array(dCount, &mr)).first->value().if_array();
			for(std::size_t i = 0; i < dCount; i++)
			{
				const auto& duelist = rp.duelists.pairs[i];
				auto& client = ac[i].emplace_object();
				client.emplace("pos", duelist.pos);
				client.emplace("name", std::string_view{duelist.name.data(), duelist.nameLength});
			}
		});
		{
			constexpr const char* const HTTP_HEADER_FORMAT_STRING =
			"HTTP/1.0 200 OK\r\n"
			"Content-Length: {:d}\r\n"
			"Content-Type: application/json\r\n\r\n";
			const auto strJ = boost::json::serialize(j); // DUMP EET
			auto full = fmt::format(HTTP_HEADER_FORMAT_STRING, strJ.size());
			full += strJ;
			std::scoped_lock lock(mSerialized);
			serialized = std::make_shared<const std::string>(std::move(full));
		}
		DoSerialize();
	});
}

void LobbyListing::DoAccept()
{
	acceptor.async_accept(
	[this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
			Workaround::SetCloseOnExec(socket.native_handle());
			std::scoped_lock lock(mSerialized);
			std::make_shared<Connection>(std::move(socket), serialized)->DoRead();
		}
		DoAccept();
	});
}

} // namespace Ignis::Multirole::Endpoint
