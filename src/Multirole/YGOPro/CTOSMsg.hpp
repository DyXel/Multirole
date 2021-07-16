#ifndef CTOSMSG_HPP
#define CTOSMSG_HPP
#include <array>
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

	inline LengthType GetLength() const noexcept
	{
		LengthType v{};
		std::memcpy(&v, bytes.data(), sizeof(LengthType));
		return v - 1U;
	}

	inline MsgType GetType() const noexcept
	{
		MsgType v{};
		std::memcpy(&v, bytes.data() + sizeof(LengthType), sizeof(MsgType));
		return v;
	}

	inline bool IsHeaderValid() const noexcept
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
	inline std::optional<s> Get##s() const noexcept \
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

	constexpr const uint8_t* Body() const noexcept
	{
		return bytes.data() + HEADER_LENGTH;
	}

	template<typename T>
	constexpr T Read(const uint8_t*& ptr) const
	{
		{
			const uint8_t* s1 = ptr + sizeof(T);
			const uint8_t* s2 = Body() + GetLength();
			if(s1 > s2) throw std::out_of_range("buffer too small");
		}
		T val;
		std::memcpy(&val, ptr, sizeof(T));
		ptr += sizeof(T);
		return val;
	}

	constexpr uint8_t* Data() noexcept
	{
		return bytes.data();
	}

	constexpr uint8_t* Body() noexcept
	{
		return bytes.data() + HEADER_LENGTH;;
	}

private:
	std::array<uint8_t, HEADER_LENGTH + MSG_MAX_LENGTH> bytes{};
};

} // namespace YGOPro

#endif // CTOSMSG_HPP
