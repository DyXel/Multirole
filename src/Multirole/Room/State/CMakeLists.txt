set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/ChoosingTurn.cpp
	${CSR}/Closing.cpp
	${CSR}/Dueling.cpp
	${CSR}/RockPaperScissor.cpp
	${CSR}/Waiting.cpp
)
unset(CSR)
