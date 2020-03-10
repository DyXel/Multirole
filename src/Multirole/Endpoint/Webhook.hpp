#ifndef WEBHOOKENDPOINT_HPP
#define WEBHOOKENDPOINT_HPP
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace Ignis::Multirole::Endpoint
{

class Webhook
{
public:
	Webhook(asio::io_context& ioCtx, unsigned short port);
	void Stop();
protected:
	virtual void Callback(std::string_view payload);
private:
	asio::ip::tcp::acceptor acceptor;

	void DoAccept();
	void DoReadHeader(asio::ip::tcp::socket soc);
};

} // namespace Ignis::Multirole::Endpoint

#endif // WEBHOOKENDPOINT_HPP
