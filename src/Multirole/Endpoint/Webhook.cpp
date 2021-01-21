#include "Webhook.hpp"

#include <thread>
#include <asio/write.hpp>

#include "../Workaround.hpp"

namespace Ignis::Multirole::Endpoint
{

// public

Webhook::Webhook(asio::io_context& ioCtx, unsigned short port) :
	acceptor(ioCtx, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port))
{
	Workaround::SetCloseOnExec(acceptor.native_handle());
	DoAccept();
}

void Webhook::Stop()
{
	acceptor.close();
}

void Webhook::Callback([[maybe_unused]] std::string_view payload)
{}

// private

void Webhook::DoAccept()
{
	acceptor.async_accept(
	[this](std::error_code ec, asio::ip::tcp::socket socket)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
			Workaround::SetCloseOnExec(socket.native_handle());
			std::make_shared<Connection>(*this, std::move(socket))->DoReadHeader();
		}
		DoAccept();
	});
}

static const auto HTTP_OK = asio::buffer(
	"HTTP/1.0 200 OK\r\n"
	"Content-Length: 17\r\n"
	"Content-Type: text/plain\r\n\r\n"
	"Payload received."
);

Webhook::Connection::Connection(Webhook& webhook, asio::ip::tcp::socket socket)
	:
	webhook(webhook),
	socket(std::move(socket)),
	incoming()
{
	incoming.fill(' ');
}

void Webhook::Connection::DoReadHeader()
{
	auto self(shared_from_this());
	socket.async_read_some(asio::buffer(incoming),
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(ec)
			return;
		incoming.back() = '\0'; // Guarantee null-terminated string.
		webhook.Callback(incoming.data());
		DoReadEnd();
		DoWrite();
	});
}

void Webhook::Connection::DoReadEnd()
{
	auto self(shared_from_this());
	socket.async_read_some(asio::buffer(incoming),
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(!ec)
			DoReadEnd();
	});
}

void Webhook::Connection::DoWrite()
{
	auto self(shared_from_this());
	asio::async_write(socket, HTTP_OK,
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(!ec)
			socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	});
}

} // namespace Ignis::Multirole::Endpoint
