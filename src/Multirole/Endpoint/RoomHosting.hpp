#ifndef ENDPOINT_ROOMHOSTING_HPP
#define ENDPOINT_ROOMHOSTING_HPP
#include <mutex>
#include <memory>
#include <set>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "../Service.hpp"
#include "../Room/Instance.hpp"
#include "../YGOPro/CTOSMsg.hpp"
#include "../YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

class Lobby;

namespace Endpoint
{

class RoomHosting final
{
public:
	enum class PrebuiltMsgId
	{
		PREBUILT_MSG_VERSION_MISMATCH = 0,
		PREBUILT_INVALID_NAME,
		PREBUILT_ROOM_NOT_FOUND,
		PREBUILT_ROOM_WRONG_PASS,
		PREBUILT_INVALID_MSG,
		PREBUILT_GENERIC_JOIN_ERROR,
		PREBUILT_KICKED_BEFORE,
		PREBUILT_MSG_COUNT
	};

	RoomHosting(boost::asio::io_context& ioCtx, Service& svc, Lobby& lobby, unsigned short port);
	void Stop();

	const YGOPro::STOCMsg& GetPrebuiltMsg(PrebuiltMsgId id) const;
	Lobby& GetLobby() const;
	Room::Instance::CreateInfo GetBaseRoomCreateInfo(uint32_t banlistHash) const;
private:
	class Connection final : public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(const RoomHosting& roomHosting, boost::asio::ip::tcp::socket socket);
		void DoReadHeader();
	private:
		enum class Status
		{
			STATUS_CONTINUE,
			STATUS_MOVED,
			STATUS_ERROR,
		};

		const RoomHosting& roomHosting;
		boost::asio::ip::tcp::socket socket;
		std::string name;
		YGOPro::CTOSMsg incoming;
		std::queue<YGOPro::STOCMsg> outgoing;

		void DoReadBody();
		void DoWrite();
		void DoReadEnd();

		Status HandleMsg();
	};

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
