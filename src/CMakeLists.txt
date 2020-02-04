set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/CardDatabase.cpp
	${CSR}/DLOpen.cpp
)
unset(CSR)

# add_subdirectory(Hornet)
add_subdirectory(Multirole)
