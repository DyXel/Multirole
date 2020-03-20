set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/BanlistProvider.cpp
	${CSR}/CardDatabase.cpp
	${CSR}/Client.cpp
	${CSR}/CoreProvider.cpp
	${CSR}/DataProvider.cpp
	${CSR}/GitRepo.cpp
	${CSR}/Instance.cpp
	${CSR}/Lobby.cpp
	${CSR}/main.cpp
	${CSR}/Room.cpp
	${CSR}/ScriptProvider.cpp
	${CSR}/STOCMsgFactory.cpp
)
unset(CSR)

add_subdirectory(Core)
add_subdirectory(Endpoint)
add_subdirectory(YGOPro)
