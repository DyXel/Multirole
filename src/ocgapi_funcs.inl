OCGFUNC(void, OCG_GetVersion, (int* major, int* minor))

OCGFUNC(int, OCG_CreateDuel, (OCG_Duel* duel, OCG_DuelOptions options))
OCGFUNC(void, OCG_DestroyDuel, (OCG_Duel duel))
OCGFUNC(void, OCG_DuelNewCard, (OCG_Duel duel, OCG_NewCardInfo info))
OCGFUNC(int, OCG_StartDuel, (OCG_Duel duel))

OCGFUNC(int, OCG_DuelProcess, (OCG_Duel duel))
OCGFUNC(void*, OCG_DuelGetMessage, (OCG_Duel duel, uint32_t* length))
OCGFUNC(void, OCG_DuelSetResponse, (OCG_Duel duel, void* buffer, uint32_t length))
OCGFUNC(int, OCG_LoadScript, (OCG_Duel duel, const char* buffer, uint32_t length, const char* name))

OCGFUNC(uint32_t, OCG_DuelQueryCount, (OCG_Duel duel, uint8_t team, uint32_t loc))
OCGFUNC(void*, OCG_DuelQuery, (OCG_Duel duel, uint32_t* length, OCG_QueryInfo info))
OCGFUNC(void*, OCG_DuelQueryLocation, (OCG_Duel duel, uint32_t* length, OCG_QueryInfo info))
OCGFUNC(void*, OCG_DuelQueryField, (OCG_Duel duel, uint32_t* length))
