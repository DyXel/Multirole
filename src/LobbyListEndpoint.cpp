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

void LobbyListEndpoint::Stop()
{
	acceptor.close();
}

void LobbyListEndpoint::Terminate()
{
	acceptor.cancel();
}

// private

std::string LobbyListEndpoint::ComposeMsg()
{
	auto ComposeHeader = [](std::size_t length, std::string_view mime)
	{
		constexpr const char* HTTP_HEADER_FORMAT_STRING =
		"HTTP/1.0 200 OK\r\n"
		"Content-Length: {}\r\n"
		"Content-Type: {}\r\n\r\n";
		return fmt::format(HTTP_HEADER_FORMAT_STRING, length, mime);
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
	auto msg = std::make_shared<std::string>(std::move(ComposeMsg()));
	asio::async_write(*socPtr, asio::buffer(*msg),
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
