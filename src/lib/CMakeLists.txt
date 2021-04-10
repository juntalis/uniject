
set(LIB_SOURCES
	base.c
	packing.c
	error.c
	ipc.c
	module.c
	params.c
	pch.c
	process.c
	utility.c
	win32.c
	injector.c
	base_private.h
	build_config.h
	error_private.h
	internal.h
	pch.h
	peutil.h
	process_private.h
)

add_compile_definitions(UNIJ_BUILD=1)
include_directories(${CMAKE_CURRENT_LIST_DIR})

add_library(uniject STATIC ${LIB_SOURCES})
add_precompiled_header(uniject pch.h FORCEINCLUDE)
set_target_properties(uniject PROPERTIES CLEAN_DIRECT_OUTPUT 1)
