# TODO
set(LOADER_SOURCES
	mono_api.c
	loader.c
	main.c
	error.c
	pch.c
	error.h
	loader.h
	mono_api.h
	mono_api.inl
	mono_types.h
	pch.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR})
add_library(uniject-loader SHARED ${LOADER_SOURCES})
set_source_files_properties(mono_api.inl PROPERTIES HEADER_FILE_ONLY ON)
add_precompiled_header(uniject-loader pch.h
	FORCEINCLUDE
	SOURCE_C pch.c
)
set_target_properties(uniject-loader PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(uniject-loader PROPERTIES OUTPUT_NAME "uniject-loader-${UNIJECT_BITS}")
target_link_libraries(uniject-loader uniject)

if (${CMAKE_BUILD_TYPE} MATCHES "Rel")
	target_link_libraries(uniject-loader ucrt)
	target_link_libraries(uniject-loader vcruntime)
else()
	target_link_libraries(uniject-loader ucrtd)
	target_link_libraries(uniject-loader vcruntimed)
endif()
