#include "DiscordWebhookSink.hpp"

#include <boost/json.hpp>
#include <fmt/format.h>

#include "../../I18N.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

class DiscordWebhookSink::Connection final : public std::enable_shared_from_this<Connection>
{
public:
	using Endpoints = boost::asio::ip::tcp::resolver::results_type;

	Connection(
		boost::asio::io_context& ioCtx,
		boost::asio::ssl::context& sslCtx,
		const Endpoints& endpoints,
		std::string payload,
		const std::string& host)
		:
		socket(ioCtx, sslCtx),
		endpoints(endpoints),
		payload(std::move(payload))
	{
		using namespace boost::asio::ssl;
		socket.set_verify_mode(verify_peer);
		socket.set_verify_callback(host_name_verification(host));
	}

	void DoConnect() noexcept
	{
		using Endpoint = Endpoints::endpoint_type;
		auto self(shared_from_this());
		boost::asio::async_connect(socket.lowest_layer(), endpoints,
		[this, self](boost::system::error_code ec, const Endpoint& /*unused*/)
		{
			if(ec)
				return;
			DoHandshake();
		});
	}
private:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
	const Endpoints& endpoints;
	const std::string payload;

	void DoHandshake() noexcept
	{
		auto self(shared_from_this());
		socket.async_handshake(decltype(socket)::client,
		[this, self](boost::system::error_code ec)
		{
			if(ec)
				return;
			DoWrite();
		});
	}

	void DoWrite() noexcept
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket, boost::asio::buffer(payload),
		[self](boost::system::error_code /*unused*/, std::size_t /*unused*/){});
	}
};

DiscordWebhookSink::DiscordWebhookSink(boost::asio::io_context& ioCtx, std::string_view uri, std::string_view ridFormat) :
	ioCtx(ioCtx),
	sslCtx(boost::asio::ssl::context::sslv23),
	ridFormat(ridFormat)
{
	sslCtx.set_default_verify_paths();
	const auto scpos = uri.find(':'); // scheme colon position
	if(scpos == std::string_view::npos)
		throw std::invalid_argument(I18N::DWH_URI_COLON_NOT_FOUND);
	const auto tspos = scpos + 1U + std::string_view::traits_type::length("//"); // two slashes position
	if(tspos > uri.length())
		throw std::invalid_argument(I18N::DWH_URI_TOO_SHORT);
	const auto pspos = uri.find('/', tspos); // path slash position
	if(pspos == std::string_view::npos)
		throw std::invalid_argument(I18N::DWH_URI_NO_PATH);
	scheme = std::string(uri.substr(0U, scpos));
	host   = std::string(uri.substr(tspos, pspos - tspos));
	path   = std::string(uri.substr(pspos));
	endpoints = boost::asio::ip::tcp::resolver(ioCtx).resolve(host, scheme);
	if(endpoints.empty())
		throw std::runtime_error(I18N::DWH_ERROR_RESOLVING_HOST);
}

DiscordWebhookSink::~DiscordWebhookSink() noexcept = default;

#include "SvcNames.inl"

constexpr std::array<
	uint32_t,
	static_cast<std::size_t>(ServiceType::SERVICE_TYPE_COUNT)
> LEVEL_COLORS =
{
	0x00FF00U,
	0xFFFF00U,
	0xFF0000U,
};

constexpr std::array<
	const char* const,
	static_cast<std::size_t>(ServiceType::SERVICE_TYPE_COUNT)
> EC_TITLES =
{
	"Core Error",
	"Official Script Error",
	"Speed Script Error",
	"Rush Script Error",
	"Unofficial Script Error",
};

template<typename T>
constexpr std::size_t AsSizeT(T v)
{
	return static_cast<std::size_t>(v);
}

constexpr const char* const HTTP_HEADER_FORMAT_STRING =
"POST {:s} HTTP/1.1\r\n"
"Host: {:s}\r\n"
"User-Agent: DyXel-Multirole/1.0\r\n"
"Content-Length: {:d}\r\n"
"Content-Type: application/json\r\n\r\n";

void DiscordWebhookSink::Log(const Timestamp& /*ts*/, const SinkLogProps& props, std::string_view str) noexcept
{
	boost::json::monotonic_resource mr;
	boost::json::object j(&mr);
	auto& embeds = *j.emplace("embeds", boost::json::array(&mr)).first->value().if_array();
	auto& embed = *embeds.emplace_back(boost::json::object(4U, &mr)).if_object();
	if(std::holds_alternative<SvcLogProps>(props))
	{
		const auto& svcLogProps = std::get<0>(props);
		embed.emplace("title", I18N::DWH_SERVICE_MESSAGE_TITLE);
		embed.emplace("description", str);
		embed.emplace("color", LEVEL_COLORS[AsSizeT(svcLogProps.second)]);
		auto& footer = *embed.emplace("footer", boost::json::object(1U, &mr)).first->value().if_object();
		footer.emplace("text", SVC_NAMES[AsSizeT(svcLogProps.first)]);
	}
	else // std::holds_alternative<ECLogProps>(props)
	{
		const auto& ecLogProps = std::get<1>(props);
		embed.emplace("title", EC_TITLES[AsSizeT(ecLogProps.first)]);
		embed.emplace("color", 0xFF0000U);
		auto& desc = *embed.emplace("description", "```\n").first->value().if_string();
		desc.append(str);
		desc.append("\n```");
		try
		{
			desc.append(fmt::format(ridFormat, ecLogProps.second));
		}
		catch(const fmt::format_error&)
		{
			desc.append(I18N::DWH_RIDFORMAT_ERROR);
		}
	}
	const auto strJ = boost::json::serialize(j); // DUMP EET
	auto payload = fmt::format(HTTP_HEADER_FORMAT_STRING, path, host, strJ.size());
	payload += strJ;
	std::make_shared<Connection>(
		ioCtx,
		sslCtx,
		endpoints,
		std::move(payload),
		host)->DoConnect();
}

} // namespace Ignis::Multirole::LogHandlerDetail
