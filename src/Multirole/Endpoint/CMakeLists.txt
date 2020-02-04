set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/LobbyListingEndpoint.cpp
	${CSR}/RoomHostingEndpoint.cpp
	${CSR}/WebhookEndpoint.cpp
)
unset(CSR)
