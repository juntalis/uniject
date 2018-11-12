
set(LIB_SOURCES
	internal.h
	base.c
	data.c
	error.c
	ipc.c
	module.c
	params.c
	pch.c
	process.c
	peutil.h
	utility.c
	win32.c base_private.h injector.c)

add_definitions("-DUNIJ_BUILD=1")
include_directories(${CMAKE_CURRENT_LIST_DIR})

add_library(uniject STATIC ${LIB_SOURCES})
add_precompiled_header(uniject pch.h FORCEINCLUDE)
set_target_properties(uniject PROPERTIES CLEAN_DIRECT_OUTPUT 1)
