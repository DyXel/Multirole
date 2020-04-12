set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/Client.cpp
	${CSR}/Context.cpp
	${CSR}/Instance.cpp
)
unset(CSR)

add_subdirectory(State)
