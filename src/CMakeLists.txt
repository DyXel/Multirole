set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/CardDatabase.cpp
	${CSR}/Client.cpp
	${CSR}/Instance.cpp
	${CSR}/Lobby.cpp
	${CSR}/LobbyListingEndpoint.cpp
	${CSR}/main.cpp
	${CSR}/Room.cpp
	${CSR}/RoomHostingEndpoint.cpp
	${CSR}/SharedCore.cpp
	${CSR}/StringUtils.cpp
	${CSR}/WebhookEndpoint.cpp
)
unset(CSR)
