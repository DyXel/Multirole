set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/Alloc.c
	${CSR}/LzFind.c
	${CSR}/LzmaEnc.c
)
unset(CSR)
