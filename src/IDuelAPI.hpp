#ifndef IDUELAPI_HPP
#define IDUELAPI_HPP
#include "ocgapi_types.hpp"

namespace Placeholder4
{

class IDuelAPI
{
public:
	virtual int OCG_CreateDuel(OCG_Duel* duel, OCG_DuelOptions options) = 0;
	virtual void OCG_DestroyDuel(OCG_Duel duel) = 0;
	virtual void OCG_DuelNewCard(OCG_Duel duel, OCG_NewCardInfo info) = 0;
	virtual void OCG_StartDuel(OCG_Duel duel) = 0;

	virtual int OCG_DuelProcess(OCG_Duel duel) = 0;
	virtual void* OCG_DuelGetMessage(OCG_Duel duel, uint32_t* length) = 0;
	virtual void OCG_DuelSetResponse(OCG_Duel duel, void* buffer, uint32_t length) = 0;
	virtual int OCG_LoadScript(OCG_Duel duel, const char* buffer, uint32_t length, const char* name) = 0;

	virtual uint32_t OCG_DuelQueryCount(OCG_Duel duel, uint8_t team, uint32_t loc) = 0;
	virtual void* OCG_DuelQuery(OCG_Duel duel, uint32_t* length, OCG_QueryInfo info) = 0;
	virtual void* OCG_DuelQueryLocation(OCG_Duel duel, uint32_t* length, OCG_QueryInfo info) = 0;
	virtual void* OCG_DuelQueryField(OCG_Duel duel, uint32_t* length) = 0;
};


} // namespace Placeholder4

#endif // IDUELAPI_HPP
