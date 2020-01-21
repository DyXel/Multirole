#include "LobbyListEndpoint.hpp"

#include <fmt/format.h> // fmt::to_string

namespace Ignis
{

// public

LobbyListEndpoint::LobbyListEndpoint(
	asio::io_context& ioContext, unsigned short port,
	std::shared_ptr<Lobby> lobby) :
	acceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
	lobby(lobby)
{
	DoAccept();
}

void LobbyListEndpoint::Terminate()
{
	acceptor.close();
}

// private

std::string LobbyListEndpoint::ComposeMsg()
{
	auto ComposeHeader = [](std::size_t length, std::string_view mime)
	{
		std::string header("HTTP/1.0 200 OK\r\n");
		header += "Content-Length: " + fmt::to_string(length) + "\r\n";
		header += "Content-Type: ";
		header += mime;
		header += "\r\n\r\n";
		return header;
	};
	std::string sJson = "{}";
	return ComposeHeader(sJson.size(), "application/json") + sJson;
}

void LobbyListEndpoint::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket soc)
	{
		DoAccept();
		if(!ec)
			DoSendRoomList(std::move(soc));
	});
}

void LobbyListEndpoint::DoSendRoomList(asio::ip::tcp::socket soc)
{
	using namespace asio::ip;
	auto socPtr = std::make_shared<tcp::socket>(std::move(soc));
	auto msg = std::make_shared<std::string>(ComposeMsg());
	socPtr->async_send(asio::buffer(msg->data(), msg->size()),
	[socPtr, msg](const std::error_code& ec, std::size_t)
	{
		if(!ec)
		{
			std::error_code ignoredEc;
			socPtr->shutdown(tcp::socket::shutdown_both, ignoredEc);
		}
	});
}

} // namespace Ignis
