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

/**
 * @brief Process attributes returned by \a unij_process_get_flags
 */
enum unij_procflags
{
	UNIJ_PROCESS_INVALID = 0,
	
	UNIJ_PROCESS_CUI = 1 << 0,
	UNIJ_PROCESS_GUI = 1 << 1,
	UNIJ_PROCESS_UI_MASK = UNIJ_PROCESS_CUI | UNIJ_PROCESS_GUI,
	
	UNIJ_PROCESS_WIN32    = 1 << 2,
	UNIJ_PROCESS_WIN64    = 1 << 3,
	UNIJ_PROCESS_BITS_MASK = UNIJ_PROCESS_WIN32 | UNIJ_PROCESS_WIN64,
	
	UNIJ_PROCESS_MASK = UNIJ_PROCESS_UI_MASK | UNIJ_PROCESS_BITS_MASK
};

/**
 * @brief Used exclusively by \a unij_enum_unity_processes
 */
struct unij_monoinfo
{
	uint32_t pid;
	const wchar_t* exe_path;
	const wchar_t* mono_name;
	const wchar_t* mono_path;
};

typedef enum unij_procflags unij_procflags_t;
typedef struct unij_monoinfo unij_monoinfo_t;

/**
 * @brief Forward declaration of our process-based struct.
 * Created with \a unij_open_process. Destroyed with \a unij_close_process
 */
typedef struct unij_process unij_process_t;

/**
 * @brief Used exclusively by \a unij_enum_unity_processes
 */
typedef bool(CDECL* unij_monoinfo_fn)(unij_monoinfo_t* info, void* parameter);

/**
 * @brief 
 * @param fn 
 * @param parameter 
 */
void unij_enum_mono_processes(unij_monoinfo_fn fn, void* parameter);

/**
 * @brief Opens a process for use with our injection functionality.
 * @param[in] pid Required process id
 * @return NULL on failure
 */
unij_process_t* unij_process_open(uint32_t pid);

/**
 * @brief Wraps a already opened process handle (and optional thread handle) for use with our injection functionality.
 * NOTE: Uses `DuplicateHandle` internally to maintain its own handle.
 * @param[in] process Required process handle
 * @return NULL on failure
 */
unij_process_t* unij_process_assign(HANDLE process);

/**
 * @brief Closes a process previously opened by \a unij_process_open/unij_process_assign.
 * @param[in] process Target process
 */
void unij_process_close(unij_process_t* process);

// Accessors

uint32_t unij_process_get_pid(unij_process_t* process);

HANDLE unij_process_get_handle(unij_process_t* process);

unij_wstr_t* unij_process_get_mono_path(unij_process_t* process);

unij_procflags_t unij_process_flags(unij_process_t* process, unij_procflags_t mask);


#define unij_process_is32bit(PROCESS) \
	(!!( unij_process_flags((PROCESS), UNIJ_PROCESS_WIN32) ) )

#define unij_process_is64bit(PROCESS) \
	( !!( unij_process_flags((PROCESS), UNIJ_PROCESS_WIN64) ) )

/**
 * @brief Extracts the detected image type from a process.
 * @param[in] process Target process 
 * @return -1 in the case of a failure. Otherwise, it returns the addressing model. (32 or 64)
 */
static UNIJ_INLINE int unij_process_bits(unij_process_t* process)
{
	switch(unij_process_flags(process, UNIJ_PROCESS_BITS_MASK))
	{
		case UNIJ_PROCESS_WIN32:
			return 32;
#		ifdef UNIJ_ARCH_X64
		case UNIJ_PROCESS_WIN64:
			return 64;
#		endif
		default:
			return -1;
	}
}

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_PROCESS_H_ */
