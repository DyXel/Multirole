#ifndef WEBHOOKENDPOINT_HPP
#define WEBHOOKENDPOINT_HPP
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace Ignis::Multirole::Endpoint
{

class Webhook
{
public:
	Webhook(boost::asio::io_context& ioCtx, unsigned short port);
	void Stop();

	virtual void Callback(std::string_view payload);
protected:
	inline ~Webhook() = default;
private:
	class Connection;

	boost::asio::ip::tcp::acceptor acceptor;

	void DoAccept();
};

} // namespace Ignis::Multirole::Endpoint

#endif // WEBHOOKENDPOINT_HPP
