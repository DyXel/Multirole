#ifndef WEBHOOKENDPOINT_HPP
#define WEBHOOKENDPOINT_HPP
#include <asio.hpp>

namespace Ignis
{

class WebhookEndpoint
{
public:
	WebhookEndpoint(asio::io_context& ioContext, unsigned short port);
	void Stop();
protected:
	virtual void Callback(std::string_view payload);
private:
	asio::ip::tcp::acceptor acceptor;

	void DoAccept();
	void DoReadHeader(asio::ip::tcp::socket soc);
};

} // namespace Ignis

#endif // WEBHOOKENDPOINT_HPP
