#include "WebhookEndpoint.hpp"

#include <thread>

namespace Ignis
{

namespace Multirole {

// public

WebhookEndpoint::WebhookEndpoint(asio::io_context& ioContext, unsigned short port) :
	acceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
	DoAccept();
}

void WebhookEndpoint::Stop()
{
	acceptor.close();
}

// protected
void WebhookEndpoint::Callback([[maybe_unused]] std::string_view payload)
{}

// private

void WebhookEndpoint::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket soc)
	{
		if(ec)
			return;
		DoAccept();
		DoReadHeader(std::move(soc));
	});
}

void WebhookEndpoint::DoReadHeader(asio::ip::tcp::socket soc)
{
	using namespace asio::ip;
	auto socPtr = std::make_shared<tcp::socket>(std::move(soc));
	auto payload = std::make_shared<std::string>(' ', 255);
	socPtr->async_read_some(asio::buffer(*payload),
	[this, socPtr, payload](const std::error_code& ec, std::size_t)
	{
		if(ec)
			return;
		asio::write(*socPtr, asio::buffer("HTTP/1.0 200 OK\r\n"));
		std::error_code ignoredEc;
		socPtr->shutdown(tcp::socket::shutdown_both, ignoredEc);
		*payload += '\0'; // Guarantee null-terminated string
		Callback(*payload);
	});
}

} // namespace Multirole

} // namespace Ignis
