set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/CardDatabase.cpp
	${CSR}/Lobby.cpp
	${CSR}/LobbyListingEndpoint.cpp
	${CSR}/main.cpp
	${CSR}/Room.cpp
	${CSR}/RoomHostingEndpoint.cpp
	${CSR}/ServerInstance.cpp
	${CSR}/SharedCore.cpp
	${CSR}/WebhookEndpoint.cpp
)
unset(CSR)
