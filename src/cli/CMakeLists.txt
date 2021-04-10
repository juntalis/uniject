# TODO

set(CLI_SOURCES
	pch.c
	main.c
	args.c
	parg.c
	args.h
	parg.h
	pch.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR})
add_executable(unijector ${CLI_SOURCES})
add_precompiled_header(unijector pch.h
	FORCEINCLUDE
	SOURCE_C pch.c
)
set_target_properties(unijector PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(unijector uniject)
