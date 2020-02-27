set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/CardDatabase.cpp
	${CSR}/Client.cpp
	${CSR}/DataProvider.cpp
	${CSR}/GitRepo.cpp
	${CSR}/Instance.cpp
	${CSR}/Lobby.cpp
	${CSR}/LoggerToStdout.cpp
	${CSR}/main.cpp
	${CSR}/Room.cpp
	${CSR}/STOCMsgFactory.cpp
)
unset(CSR)

add_subdirectory(Core)
add_subdirectory(Endpoint)
add_subdirectory(YGOPro)
