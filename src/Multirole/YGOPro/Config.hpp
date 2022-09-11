#ifndef YGOPRO_CONFIG
#define YGOPRO_CONFIG
#include "MsgCommon.hpp"

namespace YGOPro
{

// Server version used to check against connecting clients and also serialized
// to replays.
constexpr YGOPro::ClientVersion SERVER_VERSION =
{
	{
		40, // NOLINT: Client version major
		0,  // NOLINT: Client version minor
	},
	{
		10,  // NOLINT: Core version major
		0   // NOLINT: Core version minor
	}
};

// Magic value checked to match the client's for no particular reason.
constexpr uint32_t SERVER_HANDSHAKE = 4043399681U;

} // namespace YGOPro

#endif // YGOPRO_CONFIG
