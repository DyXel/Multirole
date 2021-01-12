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

// protected
void Webhook::Callback([[maybe_unused]] std::string_view payload)
{}

// private

void Webhook::DoAccept()
{
	acceptor.async_accept(
	[this](std::error_code ec, asio::ip::tcp::socket soc)
	{
		if(ec == asio::error::operation_aborted)
			return;
		if(!ec)
		{
			Workaround::SetCloseOnExec(soc.native_handle());
			DoReadHeader(std::move(soc));
		}
		DoAccept();
	});
}

void Webhook::DoReadHeader(asio::ip::tcp::socket soc)
{
	auto socPtr = std::make_shared<asio::ip::tcp::socket>(std::move(soc));
	auto payload = std::make_shared<std::string>(255, ' ');
	socPtr->async_read_some(asio::buffer(*payload),
	[this, socPtr, payload](std::error_code ec, std::size_t /*unused*/)
	{
		if(ec)
			return;
		asio::write(*socPtr, asio::buffer("HTTP/1.0 200 OK\r\n"));
		*payload += '\0'; // Guarantee null-terminated string
		Callback(*payload);
	});
}

} // namespace Ignis::Multirole::Endpoint
