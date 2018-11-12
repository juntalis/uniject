/**
 * @file uniject/process.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for process.h
 */
#ifndef _UNIJECT_PROCESS_H_
#define _UNIJECT_PROCESS_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

struct unij_unity_process
{
	uint32_t pid;
	const wchar_t* exe_path;
	const wchar_t* mono_name;
};

typedef struct unij_unity_process unij_unity_process_t;

typedef bool(CDECL* unij_enum_process_fn)(unij_unity_process_t* process, void* parameter);

enum unij_process_flags
{
	UNIJ_PROCESS_INVALID = 0,
	
	UNIJ_PROCESS_CUI = 1 << 0,
	UNIJ_PROCESS_GUI = 1 << 1,
	UNIJ_PROCESS_UI_MASK = UNIJ_PROCESS_CUI | UNIJ_PROCESS_GUI,
	
	UNIJ_PROCESS_32    = 1 << 2,
	UNIJ_PROCESS_64    = 1 << 3,
	UNIJ_PROCESS_WOW64 = UNIJ_PROCESS_32 | UNIJ_PROCESS_64
};

typedef enum unij_process_flags unij_process_flags_t;

unij_process_flags_t unij_get_process_type(LPPROCESS_INFORMATION ppi, uint32_t* pBase);
void unij_enum_unity_processes(unij_enum_process_fn fn, void* parameter);

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_PROCESS_H_ */
