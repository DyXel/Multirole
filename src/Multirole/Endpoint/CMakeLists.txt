set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/LobbyListing.cpp
	${CSR}/RoomHosting.cpp
	${CSR}/Webhook.cpp
)
unset(CSR)
