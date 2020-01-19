set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/CardDatabase.cpp
	${CSR}/main.cpp
	${CSR}/ServerInstance.cpp
	${CSR}/SharedCore.cpp
)
unset(CSR)
