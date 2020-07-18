set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/Banlist.cpp
	${CSR}/CoreUtils.cpp
	${CSR}/Deck.cpp
	${CSR}/Replay.cpp
	${CSR}/StringUtils.cpp
)
unset(CSR)
