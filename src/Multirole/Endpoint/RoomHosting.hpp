#ifndef ROOMHOSTINGENDPOINT_HPP
#define ROOMHOSTINGENDPOINT_HPP
#include <mutex>
#include <memory>
#include <set>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "../Room/Instance.hpp"
#include "../YGOPro/CTOSMsg.hpp"
#include "../YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

class BanlistProvider;
class CoreProvider;
class DataProvider;
class ReplayManager;
class ScriptProvider;
class Lobby;

namespace Endpoint
{

class RoomHosting final
{
public:
	// Data passed on the ctor.
	struct CreateInfo
	{
		asio::io_context& ioCtx;
		unsigned short port;
		BanlistProvider& banlistProvider;
		CoreProvider& coreProvider;
		DataProvider& dataProvider;
		ReplayManager& replayManager;
		ScriptProvider& scriptProvider;
		Lobby& lobby;
	};

	enum class PrebuiltMsgId
	{
		PREBUILT_MSG_VERSION_MISMATCH = 0,
		PREBUILT_INVALID_NAME,
		PREBUILT_ROOM_NOT_FOUND,
		PREBUILT_ROOM_WRONG_PASS,
		PREBUILT_INVALID_MSG,
		PREBUILT_GENERIC_JOIN_ERROR,
		PREBUILT_MSG_COUNT
	};

	RoomHosting(CreateInfo&& info);
	void Stop();

	const YGOPro::STOCMsg& GetPrebuiltMsg(PrebuiltMsgId id) const;
	std::shared_ptr<Room::Instance> GetRoomById(uint32_t id) const;
	Room::Instance::CreateInfo GetBaseRoomCreateInfo(uint32_t banlistHash) const;
private:
	class Connection final : public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(const RoomHosting& roomHosting, asio::ip::tcp::socket socket);
		void DoReadHeader();
	private:
		enum class Status
		{
			STATUS_CONTINUE,
			STATUS_MOVED,
			STATUS_ERROR,
		};

		const RoomHosting& roomHosting;
		asio::ip::tcp::socket socket;
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
	asio::io_context& ioCtx;
	asio::ip::tcp::acceptor acceptor;
	BanlistProvider& banlistProvider;
	CoreProvider& coreProvider;
	DataProvider& dataProvider;
	ReplayManager& replayManager;
	ScriptProvider& scriptProvider;
	Lobby& lobby;

	void DoAccept();
};

} // namespace Endpoint

} // namespace Ignis::Multirole

#endif // ROOMHOSTINGENDPOINT_HPP
