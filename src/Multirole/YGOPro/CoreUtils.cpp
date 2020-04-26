#include "CoreUtils.hpp"

#include <stdexcept> // std::out_of_range

#include "Msgs.inl"

namespace YGOPro::CoreUtils
{

std::vector<Msg> SplitToMsgs(const Buffer& buffer)
{
	using length_t = uint32_t;
	static constexpr std::size_t sizeOfLength = sizeof(length_t);
	std::vector<Msg> msgs;
	if(buffer.empty())
		return msgs;
	const std::size_t bufSize = buffer.size();
	const uint8_t* const bufData = buffer.data();
	for(std::size_t pos = 0; pos != bufSize; )
	{
		// Retrieve length of this message
		if(pos + sizeOfLength > bufSize)
			throw std::out_of_range("SplitMessages: Reading length");
		length_t length;
		std::memcpy(&length, bufData + pos, sizeOfLength);
		pos += sizeOfLength;
		// Copy message data to a new message
		if(pos + length > bufSize)
			throw std::out_of_range("SplitMessages: Reading message data");
		auto& msg = msgs.emplace_back(length);
		std::memcpy(msg.data(), bufData + pos, length);
		pos += length;
	}
	return msgs;
}

uint8_t GetMessageType(const Msg& msg)
{
	return msg.at(0);
}

MsgDistType GetMessageDistributionType(const Msg& msg)
{
	return MsgDistType::MSG_DIST_TYPE_FOR_EVERYONE_WITHOUT_STRIPPING; // TODO
}

uint8_t GetMessageReceivingTeam(const Msg& msg)
{
	return msg.at(1);
}

StrippedMsg StripMessageForTeam(uint8_t team, const Msg& msg)
{
	return msg; // TODO
}

const Msg& MsgFromStrippedMsg(const StrippedMsg& sMsg)
{
	if(const auto i = sMsg.index(); i == 0)
		return *std::get<0>(sMsg);
	else
		return std::get<1>(sMsg);
}

STOCMsg GameMsgFromMsg(const Msg& msg)
{
	return STOCMsg{STOCMsg::MsgType::GAME_MSG, msg};
}

} // namespace YGOPro::CoreUtils
