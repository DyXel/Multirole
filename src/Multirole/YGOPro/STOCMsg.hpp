#ifndef YGOPRO_STOCMSG_HPP
#define YGOPRO_STOCMSG_HPP
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

#include "MsgCommon.hpp"

namespace YGOPro
{

#include "../../Write.inl"

class STOCMsg
{
public:
	using LengthType = uint16_t;
	enum class MsgType : uint8_t
	{
		GAME_MSG      = 0x1,
		ERROR_MSG     = 0x2,
		CHOOSE_RPS    = 0x3,
		CHOOSE_ORDER  = 0x4,
		RPS_RESULT    = 0x5,
		ORDER_RESULT  = 0x6,
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
		PLAYER_ENTER  = 0x20,
		PLAYER_CHANGE = 0x21,
		WATCH_CHANGE  = 0x22,
		NEW_REPLAY    = 0x30,
		CATCHUP       = 0xF0,
		REMATCH       = 0xF1,
		REMATCH_WAIT  = 0xF2,
		CHAT_2        = 0xF3,
	};
	static constexpr std::size_t MAX_PAYLOAD_SIZE =
		std::numeric_limits<LengthType>::max() -
		sizeof(LengthType) - sizeof(MsgType);

	struct ErrorMsg
	{
		static constexpr auto MSG_TYPE = MsgType::ERROR_MSG;
		uint8_t msg;
		uint32_t code;
	};

	struct DeckErrorMsg
	{
		static constexpr auto MSG_TYPE = MsgType::ERROR_MSG;
		uint8_t msg;
		uint32_t type;
		struct
		{
			uint32_t got;
			uint32_t min;
			uint32_t max;
		} count;
		uint32_t code;
	};

	struct VerErrorMsg
	{
		static constexpr auto MSG_TYPE = MsgType::ERROR_MSG;
		uint8_t msg;
		char : 8U; // Padding to keep the client version
		char : 8U; // in the same place as the other
		char : 8U; // error codes.
		ClientVersion version;
	};

	struct RPSResult
	{
		static constexpr auto MSG_TYPE = MsgType::RPS_RESULT;
		uint8_t res0;
		uint8_t res1;
	};

	struct CreateGame
	{
		static constexpr auto MSG_TYPE = MsgType::CREATE_GAME;
		uint32_t id;
	};

	struct TypeChange
	{
		static constexpr auto MSG_TYPE = MsgType::TYPE_CHANGE;
		uint8_t type;
	};

	struct JoinGame
	{
		static constexpr auto MSG_TYPE = MsgType::JOIN_GAME;
		HostInfo info;
	};

	struct TimeLimit
	{
		static constexpr auto MSG_TYPE = MsgType::TIME_LIMIT;
		uint8_t team;
		uint16_t timeLeft;
	};

	struct PlayerEnter
	{
		static constexpr auto MSG_TYPE = MsgType::PLAYER_ENTER;
		uint16_t name[20U];
		uint8_t pos;
	};

	struct PlayerChange
	{
		static constexpr auto MSG_TYPE = MsgType::PLAYER_CHANGE;
		uint8_t status;
	};

	struct WatchChange
	{
		static constexpr auto MSG_TYPE = MsgType::WATCH_CHANGE;
		uint16_t count;
	};

	struct CatchUp
	{
		static constexpr auto MSG_TYPE = MsgType::CATCHUP;
		uint8_t catchingUp;
	};

	struct Chat2
	{
		static constexpr auto MSG_TYPE = MsgType::CHAT_2;
		enum PlayerType : uint8_t
		{
			PTYPE_DUELIST,
			PTYPE_OBS,
			PTYPE_SYSTEM,
			PTYPE_SYSTEM_ERROR,
			PTYPE_SYSTEM_SHOUT,
		};
		uint8_t type;
		uint8_t isTeam;
		uint16_t clientName[20U];
		uint16_t msg[256U];
	};

	STOCMsg() = delete;

	~STOCMsg() noexcept
	{
		DestroyUnion();
	}

	template<typename T>
	STOCMsg(const T& msg) noexcept
	{
		const std::size_t msgSize = sizeof(MsgType) + sizeof(T);
		uint8_t* ptr = ConstructUnionAndGetPtr(sizeof(LengthType) + msgSize);
		Write(ptr, static_cast<LengthType>(msgSize));
		Write(ptr, static_cast<MsgType>(T::MSG_TYPE));
		Write(ptr, msg);
	}

	STOCMsg(MsgType type) noexcept
	{
		const std::size_t msgSize = sizeof(MsgType);
		uint8_t* ptr = ConstructUnionAndGetPtr(sizeof(LengthType) + msgSize);
		Write(ptr, static_cast<LengthType>(msgSize));
		Write<MsgType>(ptr, type);
	}

	STOCMsg(MsgType type, const uint8_t* data, std::size_t size) noexcept
	{
		const std::size_t msgSize = sizeof(MsgType) + size;
		uint8_t* ptr = ConstructUnionAndGetPtr(sizeof(LengthType) + msgSize);
		Write(ptr, static_cast<LengthType>(msgSize));
		Write<MsgType>(ptr, type);
		std::memcpy(ptr, data, size);
	}

	template<typename ContiguousContainer>
	STOCMsg(MsgType type, const ContiguousContainer& msg) noexcept :
		STOCMsg(type, msg.data(), msg.size())
	{}

	STOCMsg(const STOCMsg& other) noexcept // Copy constructor
	{
		assert(this != &other);
		if(IsStackArray(this->length = other.length))
			new (&this->stackA) StackArray(other.stackA);
		else
			new (&this->refCntA) RefCntArray(other.refCntA);
	}

	STOCMsg& operator=(const STOCMsg& other) noexcept // Copy assignment
	{
		assert(this != &other);
		DestroyUnion();
		if(IsStackArray(this->length = other.length))
			this->stackA = other.stackA;
		else
			this->refCntA = other.refCntA;
		return *this;
	}

	STOCMsg(STOCMsg&& other) noexcept // Move constructor
	{
		assert(this != &other);
		if(IsStackArray(this->length = other.length))
			new (&this->stackA) StackArray(std::move(other.stackA));
		else
			new (&this->refCntA) RefCntArray(std::move(other.refCntA));
	}

	STOCMsg& operator=(STOCMsg&& other) noexcept // Move assignment
	{
		assert(this != &other);
		DestroyUnion();
		if(IsStackArray(this->length = other.length))
			this->stackA = std::move(other.stackA);
		else
			this->refCntA = std::move(other.refCntA);
		return *this;
	}

	std::size_t Length() const noexcept
	{
		return length;
	}

	const uint8_t* Data() const noexcept
	{
		if(IsStackArray(length))
			return stackA.data();
		else
			return refCntA.get();
	}

private:
	// Reference counted dynamic array.
	using RefCntArray = std::shared_ptr<uint8_t[]>;

	// Stack array that has the same size as RefCntArray.
	using StackArray = std::array<uint8_t, sizeof(RefCntArray)>;

	// Make sure they are both the same length.
	static_assert(sizeof(RefCntArray) == sizeof(StackArray));

	std::size_t length;
	union
	{
		StackArray stackA;
		RefCntArray refCntA;
	};

	constexpr bool IsStackArray(std::size_t size) const noexcept
	{
		return size <= std::tuple_size<StackArray>::value;
	}

	inline uint8_t* ConstructUnionAndGetPtr(std::size_t size) noexcept
	{
		if(IsStackArray(length = size))
		{
			new (&stackA) StackArray{0U};
			return stackA.data();
		}
		else
		{
			new (&refCntA) RefCntArray{new uint8_t[size]{0U}};
			return refCntA.get();
		}
	}

	inline void DestroyUnion() noexcept
	{
		if(length <= std::tuple_size<StackArray>::value)
			stackA.~StackArray();
		else
			refCntA.~RefCntArray();
	}
};

} // namespace YGOPro

#endif // YGOPRO_STOCMSG_HPP
