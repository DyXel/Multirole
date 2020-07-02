#ifndef CTOSMSG_HPP
#define CTOSMSG_HPP
#include <optional>
#include "MsgCommon.hpp"

namespace YGOPro
{

class CTOSMsg
{
public:
	enum
	{
		HEADER_LENGTH = 3U,
		MSG_MAX_LENGTH = 1021U
	};
	using LengthType = int16_t;
	enum class MsgType : uint8_t
	{
		RESPONSE      = 0x01,
		UPDATE_DECK   = 0x02,
		RPS_CHOICE    = 0x03,
		TURN_CHOICE   = 0x04,
		PLAYER_INFO   = 0x10,
		CREATE_GAME   = 0x11,
		JOIN_GAME     = 0x12,
		LEAVE_GAME    = 0x13,
		SURRENDER     = 0x14,
		TIME_CONFIRM  = 0x15,
		CHAT          = 0x16,
		TO_DUELIST    = 0x20,
		TO_OBSERVER   = 0x21,
		READY         = 0x22,
		NOT_READY     = 0x23,
		TRY_KICK      = 0x24,
		TRY_START     = 0x25,
		REMATCH       = 0xF0,
	};

	struct RPSChoice
	{
		uint8_t value;
	};

	struct TurnChoice
	{
		uint8_t value;
	};

	struct PlayerInfo
	{
		uint16_t name[20U];
	};

	struct CreateGame
	{
		HostInfo hostInfo;
		uint16_t name[20U];
		uint16_t pass[20U];
		char notes[200U];
	};

	struct JoinGame
	{
		uint16_t version2;
		uint32_t id;
		uint16_t pass[20U];
		ClientVersion version;
	};

	struct TryKick
	{
		uint8_t pos;
	};

	struct Rematch
	{
		uint8_t answer;
	};

	CTOSMsg() : r(data + HEADER_LENGTH)
	{}

	LengthType GetLength() const
	{
		LengthType v;
		std::memcpy(&v, data, sizeof(LengthType));
		return v - 1U;
	}

	MsgType GetType() const
	{
		MsgType v;
		std::memcpy(&v, data + sizeof(LengthType), sizeof(MsgType));
		return v;
	}

	bool IsHeaderValid() const
	{
		if(GetLength() > MSG_MAX_LENGTH)
			return false;
		switch(GetType())
		{
		case MsgType::RESPONSE:
		case MsgType::UPDATE_DECK:
		case MsgType::RPS_CHOICE:
		case MsgType::TURN_CHOICE:
		case MsgType::PLAYER_INFO:
		case MsgType::CREATE_GAME:
		case MsgType::JOIN_GAME:
		case MsgType::LEAVE_GAME:
		case MsgType::SURRENDER:
		case MsgType::TIME_CONFIRM:
		case MsgType::CHAT:
		case MsgType::TO_DUELIST:
		case MsgType::TO_OBSERVER:
		case MsgType::READY:
		case MsgType::NOT_READY:
		case MsgType::TRY_KICK:
		case MsgType::TRY_START:
		case MsgType::REMATCH:
			return true;
		default:
			return false;
		}
	}

#define X(s) \
	std::optional<s> Get##s() const \
	{ \
		std::optional<s> p; \
		if(GetLength() != sizeof(s)) \
			return p; \
		p.emplace(); \
		std::memcpy(&p.value(), Body(), sizeof(s)); \
		return p; \
	}
	X(RPSChoice)
	X(TurnChoice)
	X(PlayerInfo)
	X(CreateGame)
	X(JoinGame)
	X(TryKick)
	X(Rematch)
#undef X

	const uint8_t* Body() const
	{
		return data + HEADER_LENGTH;
	}

	template<typename T>
	inline T Read(const uint8_t*& ptr) const
	{
		{
			const uint8_t* s1 = ptr + sizeof(T);
			const uint8_t* s2 = Body() + GetLength();
			if(s1 > s2) throw uintptr_t(s1 - s2);
		}
		T val;
		std::memcpy(&val, ptr, sizeof(T));
		ptr += sizeof(T);
		return val;
	}

	uint8_t* Data()
	{
		return data;
	}

	uint8_t* Body()
	{
		return data + HEADER_LENGTH;
	}

private:
	uint8_t data[HEADER_LENGTH + MSG_MAX_LENGTH];
	uint8_t* r;
};

} // namespace YGOPro

#endif // CTOSMSG_HPP
