#ifndef STOCMSG_HPP
#define STOCMSG_HPP
#include "MsgCommon.hpp"

namespace YGOPro
{

class STOCMsg
{
public:
	enum
	{
		HEADER_LENGTH = 3,
	};
	using LengthType = int16_t;
	enum class MsgType : int8_t
	{
		GAME_MSG         = 0x1,
		ERROR_MSG        = 0x2,
		SELECT_HAND      = 0x3,
		SELECT_T         = 0x4,
		HAND_RESULT      = 0x5,
		TP_RESULT        = 0x6,
		CHANGE_SIDE      = 0x7,
		WAITING_SIDE     = 0x8,
		CREATE_GAME      = 0x11,
		JOIN_GAME        = 0x12,
		TYPE_CHANGE      = 0x13,
		LEAVE_GAME       = 0x14,
		DUEL_START       = 0x15,
		DUEL_END         = 0x16,
		REPLAY           = 0x17,
		TIME_LIMIT       = 0x18,
		CHAT             = 0x19,
		HS_PLAYER_ENTER  = 0x20,
		HS_PLAYER_CHANGE = 0x21,
		HS_WATCH_CHANGE  = 0x22,
	};

	const uint8_t* Data() const
	{
		return bytes.data();
	}

	std::size_t Length() const
	{
		return bytes.size();
	}

	template<typename T>
	void Write(MsgType type, const T& msg)
	{
		const auto sizeOfT = static_cast<LengthType>(sizeof(T));
		bytes.resize(sizeOfT + HEADER_LENGTH);
		uint8_t* p = bytes.data();
		std::memcpy(p, &sizeOfT, sizeof(sizeOfT));
		p += sizeof(sizeOfT);
		std::memcpy(p, &type, sizeof(MsgType));
		p += sizeof(MsgType);
		std::memcpy(p, &msg, sizeof(T));
	}

	void Write(MsgType type, const std::vector<uint8_t>& msg)
	{
		const auto msgSize = static_cast<LengthType>(msg.size());
		bytes.resize(msgSize + HEADER_LENGTH);
		uint8_t* p = bytes.data();
		std::memcpy(p, &msgSize, sizeof(msgSize));
		p += sizeof(msgSize);
		std::memcpy(p, &type, sizeof(MsgType));
		p += sizeof(MsgType);
		std::memcpy(p, msg.data(), msgSize);
	}
private:
	std::vector<uint8_t> bytes;
};

} // namespace YGOPro

#endif // STOCMSG_HPP
