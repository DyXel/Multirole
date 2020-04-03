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
		HEADER_LENGTH = 3,
		MSG_MAX_LENGTH = 1021
	};
	using LengthType = int16_t;
	enum class MsgType : int8_t
	{
		RESPONSE      = 0x01,
		UPDATE_DECK   = 0x02,
		HAND_RESULT   = 0x03,
		TP_RESULT     = 0x04,
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
		TRY_START     = 0x25
	};

	struct PlayerInfo
	{
		uint16_t name[20];
	};

	struct CreateGame
	{
		HostInfo info;
		uint16_t name[20];
		uint16_t pass[20];
		char notes[200];
	};

	struct JoinGame
	{
		uint16_t version;
		uint32_t id;
		uint16_t pass[20];
		uint32_t version2;
	};

	struct TryKick
	{
		uint8_t pos;
	};

	CTOSMsg() : r(data + HEADER_LENGTH)
	{}

	LengthType GetLength() const
	{
		LengthType v;
		std::memcpy(&v, data, sizeof(LengthType));
		return v - 1u;
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
		if(GetType() > MsgType::TRY_START)
			return false;
		return true;
	}

#define X(s) \
	std::optional<s> Get##s() const \
	{ \
		std::optional<s> p; \
		if(GetLength() != sizeof(s)) \
			return p; \
		p.emplace(); \
		std::memcpy(&p.value(), data + HEADER_LENGTH, sizeof(s)); \
		return p; \
	}
	X(PlayerInfo)
	X(CreateGame)
	X(JoinGame)
	X(TryKick)
#undef X

	const uint8_t* Body() const
	{
		return data + HEADER_LENGTH;
	}

	template<typename T>
	inline T Read(const uint8_t*& ptr) const
	{
		if(ptr + sizeof(T) > data + GetLength())
			throw std::size_t(ptr + sizeof(T) - data + GetLength());
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
