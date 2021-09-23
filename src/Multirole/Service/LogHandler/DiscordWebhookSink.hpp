#ifndef MULTIROLE_SERVICE_LOGHANDLER_DISCORDWEBHOOKSINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_DISCORDWEBHOOKSINK_HPP
#include "ISink.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace Ignis::Multirole::LogHandlerDetail
{

class DiscordWebhookSink final : public ISink
{
public:
	DiscordWebhookSink(boost::asio::io_context& ioCtx, std::string_view uri, std::string_view ridFormat);
	~DiscordWebhookSink() noexcept;

	void Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept override;
private:
	class Connection;

	boost::asio::io_context& ioCtx;
	boost::asio::ssl::context sslCtx;
	const std::string ridFormat;
	std::string scheme;
	std::string host;
	std::string path;
	boost::asio::ip::tcp::resolver::results_type endpoints;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_DISCORDWEBHOOKSINK_HPP
