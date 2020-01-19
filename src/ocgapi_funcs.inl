OCGFUNC(void, OCG_GetVersion, (int* major, int* minor), (major, minor))

OCGFUNC(int, OCG_CreateDuel, (OCG_Duel* duel, OCG_DuelOptions options), (duel, options))
OCGFUNC(void, OCG_DestroyDuel, (OCG_Duel duel), (duel))
OCGFUNC(void, OCG_DuelNewCard, (OCG_Duel duel, OCG_NewCardInfo info), (duel, info))
OCGFUNC(int, OCG_StartDuel, (OCG_Duel duel), (duel))

OCGFUNC(int, OCG_DuelProcess, (OCG_Duel duel), (duel))
OCGFUNC(void*, OCG_DuelGetMessage, (OCG_Duel duel, uint32_t* length), (duel, length))
OCGFUNC(void, OCG_DuelSetResponse, (OCG_Duel duel, void* buffer, uint32_t length), (duel, buffer, length))
OCGFUNC(int, OCG_LoadScript, (OCG_Duel duel, const char* buffer, uint32_t length, const char* name), (duel, buffer, length, name))

OCGFUNC(uint32_t, OCG_DuelQueryCount, (OCG_Duel duel, uint8_t team, uint32_t loc), (duel, team, loc))
OCGFUNC(void*, OCG_DuelQuery, (OCG_Duel duel, uint32_t* length, OCG_QueryInfo info), (duel, length, info))
OCGFUNC(void*, OCG_DuelQueryLocation, (OCG_Duel duel, uint32_t* length, OCG_QueryInfo info), (duel, length, info))
OCGFUNC(void*, OCG_DuelQueryField, (OCG_Duel duel, uint32_t* length), (duel, length))
