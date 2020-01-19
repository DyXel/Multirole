set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/main.cpp
	${CSR}/ServerInstance.cpp
)
unset(CSR)
