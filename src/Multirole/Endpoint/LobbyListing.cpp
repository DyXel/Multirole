#include "LobbyListing.hpp"

#include <boost/asio/write.hpp>
#include <boost/json.hpp>
#include <fmt/format.h> // fmt::to_string

#include "../Lobby.hpp"
#include "../Workaround.hpp"

namespace Ignis::Multirole::Endpoint
{

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
	DoAccept();
	DoSerialize();
}

// NOTE: Without this debug builds in visual studio crash without reason.
LobbyListing::~LobbyListing()
{}

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
		auto ComposeHeader = [](std::size_t length, std::string_view mime)
		{
			constexpr const char* HTTP_HEADER_FORMAT_STRING =
			"HTTP/1.0 200 OK\r\n"
			"Content-Length: {:d}\r\n"
			"Content-Type: {:s}\r\n\r\n";
			return fmt::format(HTTP_HEADER_FORMAT_STRING, length, mime);
		};
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
			room.emplace("no_check", static_cast<bool>(hi.dontCheckDeck));
			room.emplace("no_shuffle", static_cast<bool>(hi.dontShuffleDeck));
			room.emplace("banlist_hash", hi.banlistHash);
			room.emplace("istart", rp.started ? "start" : "waiting");
			auto& ac = *room.emplace("users", boost::json::array(rp.duelists.size(), &mr)).first->value().if_array();
			std::size_t i = 0U;
			for(const auto& kv : rp.duelists)
			{
				auto& client = ac[i].emplace_object();
				client.emplace("name", kv.second);
				client.emplace("pos", kv.first);
				i++;
			}
		});
		const std::string strJ = boost::json::serialize(j); // DUMP EET
		{
			std::scoped_lock lock(mSerialized);
			serialized = std::make_shared<std::string>(
				ComposeHeader(strJ.size(), "application/json") + strJ
			);
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

LobbyListing::Connection::Connection(
	boost::asio::ip::tcp::socket socket,
	std::shared_ptr<std::string> data)
	:
	socket(std::move(socket)),
	outgoing(std::move(data)),
	incoming(),
	writeCalled(false)
{}

void LobbyListing::Connection::DoRead()
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

void LobbyListing::Connection::DoWrite()
{
	auto self(shared_from_this());
	boost::asio::async_write(socket, boost::asio::buffer(*outgoing),
	[this, self](boost::system::error_code ec, std::size_t /*unused*/)
	{
		if(!ec)
			socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	});
}

} // namespace Ignis::Multirole::Endpoint
