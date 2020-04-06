#ifndef STOCMSG_HPP
#define STOCMSG_HPP
#include <cstring>
#include <type_traits>
#include <vector>

#include "MsgCommon.hpp"

namespace YGOPro
{

class STOCMsg
{
public:
	enum
	{
		HEADER_LENGTH = 2,
	};
	using LengthType = int16_t;
	enum class MsgType : int8_t
	{
		GAME_MSG      = 0x1,
		ERROR_MSG     = 0x2,
		CHOOSE_RPS    = 0x3,
		SELECT_TP     = 0x4,
		HAND_RESULT   = 0x5,
		TP_RESULT     = 0x6,
		CHANGE_SIDE   = 0x7,
		WAITING_SIDE  = 0x8,
		CREATE_GAME   = 0x11,
		JOIN_GAME     = 0x12,
		TYPE_CHANGE   = 0x13,
		LEAVE_GAME    = 0x14,
		DUEL_START    = 0x15,
		DUEL_END      = 0x16,
		REPLAY        = 0x17,
		TIME_LIMIT    = 0x18,
		CHAT          = 0x19,
		PLAYER_ENTER  = 0x20,
		PLAYER_CHANGE = 0x21,
		WATCH_CHANGE  = 0x22,
	};

	struct ErrorMsg
	{
		static const auto val = MsgType::ERROR_MSG;
		uint8_t msg;
		uint32_t code;
	};

	struct CreateGame
	{
		static const auto val = MsgType::CREATE_GAME;
		uint32_t id;
	};

	struct TypeChange
	{
		static const auto val = MsgType::TYPE_CHANGE;
		uint8_t type;
	};

	struct JoinGame
	{
		static const auto val = MsgType::JOIN_GAME;
		HostInfo info;
	};

	struct Chat
	{
		static const auto val = MsgType::CHAT;
		uint16_t posOrType;
		uint16_t msg[256];
	};

	struct PlayerEnter
	{
		static const auto val = MsgType::PLAYER_ENTER;
		uint16_t name[20];
		uint8_t pos;
	};

	struct PlayerChange
	{
		static const auto val = MsgType::PLAYER_CHANGE;
		uint8_t status;
	};

	struct WatchChange
	{
		static const auto val = MsgType::WATCH_CHANGE;
		uint16_t count;
	};

	template<typename T>
	STOCMsg(const T& msg)
	{
		static_assert(std::is_same_v<std::remove_cv_t<decltype(T::val)>, MsgType>);
		const auto msgSize = static_cast<LengthType>(sizeof(T) + sizeof(MsgType));
		bytes.resize(HEADER_LENGTH + msgSize);
		uint8_t* p = bytes.data();
		std::memcpy(p, &msgSize, sizeof(msgSize));
		p += sizeof(msgSize);
		std::memcpy(p, &T::val, sizeof(MsgType));
		p += sizeof(MsgType);
		std::memcpy(p, &msg, sizeof(T));
	}

	STOCMsg(MsgType type, const std::vector<uint8_t>& msg)
	{
		const auto msgSize = static_cast<LengthType>(msg.size() + sizeof(MsgType));
		bytes.resize(HEADER_LENGTH + msgSize);
		uint8_t* p = bytes.data();
		std::memcpy(p, &msgSize, sizeof(msgSize));
		p += sizeof(msgSize);
		std::memcpy(p, &type, sizeof(MsgType));
		p += sizeof(MsgType);
		std::memcpy(p, msg.data(), msg.size());
	}

	STOCMsg(MsgType type, [[maybe_unused]] bool tag)
	{
		const auto msgSize = static_cast<LengthType>(1 + sizeof(MsgType));
		bytes.resize(HEADER_LENGTH + msgSize);
		uint8_t* p = bytes.data();
		std::memcpy(p, &msgSize, sizeof(msgSize));
		p += sizeof(msgSize);
		std::memcpy(p, &type, sizeof(MsgType));
	}

	const uint8_t* Data() const
	{
		return bytes.data();
	}

	std::size_t Length() const
	{
		return bytes.size();
	}

	STOCMsg& Shrink(LengthType length)
	{
		length += sizeof(MsgType);
		LengthType cLength{};
		std::memcpy(&cLength, bytes.data(), sizeof(LengthType));
		if(length >= cLength)
			return *this;
		std::memcpy(bytes.data(), &length, sizeof(LengthType));
		bytes.resize(bytes.size() - (cLength - length));
		return *this;
	}
private:
	std::vector<uint8_t> bytes;
};

} // namespace YGOPro

#endif // STOCMSG_HPP
