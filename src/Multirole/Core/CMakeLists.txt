set(CSR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(multirole PRIVATE
	${CSR}/DynamicLinkWrapper.cpp
)
unset(CSR)
