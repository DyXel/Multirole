#ifndef ENDPOINT_ROOMHOSTING_HPP
#define ENDPOINT_ROOMHOSTING_HPP
#include <mutex>
#include <memory>
#include <set>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "../Service.hpp"
#include "../YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

class Lobby;

namespace Endpoint
{

class RoomHosting final
{
public:
	RoomHosting(boost::asio::io_context& ioCtx, Service& svc, Lobby& lobby, unsigned short port);
	void Stop() noexcept;
private:
	enum class PrebuiltMsgId
	{
		PREBUILT_MSG_VERSION_MISMATCH = 0,
		PREBUILT_INVALID_NAME,
		PREBUILT_ROOM_NOT_FOUND,
		PREBUILT_ROOM_WRONG_PASS,
		PREBUILT_INVALID_MSG,
		PREBUILT_GENERIC_JOIN_ERROR,
		PREBUILT_KICKED_BEFORE,
		PREBUILT_CANNOT_RESOLVE_IP,
		PREBUILT_MAX_CONNECTION_REACHED,
		PREBUILT_MSG_COUNT
	};

	class Connection;
	friend class Connection;

	const std::array<
		YGOPro::STOCMsg,
		static_cast<std::size_t>(PrebuiltMsgId::PREBUILT_MSG_COUNT)
	> prebuiltMsgs;
	boost::asio::io_context& ioCtx;
	Service& svc;
	Lobby& lobby;
	boost::asio::ip::tcp::acceptor acceptor;

	void DoAccept();
};

} // namespace Endpoint

} // namespace Ignis::Multirole

#endif // ENDPOINT_ROOMHOSTING_HPP
