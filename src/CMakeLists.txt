set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/CardDatabase.cpp
	${CSR}/Lobby.cpp
	${CSR}/LobbyListEndpoint.cpp
	${CSR}/main.cpp
	${CSR}/Room.cpp
	${CSR}/ServerInstance.cpp
	${CSR}/SharedCore.cpp
)
unset(CSR)
