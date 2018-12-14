/**
 * @file uniject/win32.h
 * 
 * Miscellaneous Win32 API helpers. Threading/synchronization, process tokens, & memory mapping. 
 */
#ifndef _UNIJECT_WIN32_H_
#define _UNIJECT_WIN32_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Object type (either mapping or evenT)
 */
enum unij_object
{
	UNIJ_OBJECT_MAPPING = 0,
	UNIJ_OBJECT_EVENT
};

typedef enum unij_object unij_object_t;

/**
 * @brief One-time initialization structure.
 * Since `InitOnceExecuteOnce` wasn't added until Windows Vista, we use our own implementation for
 * Windows XP compatibility. (and because I already wrote this for some other project)
 */
struct unij_once
{
	UNIJ_CACHE_ALIGN
	void* reserved0;
	int32_t reserved1[2];
};

typedef struct unij_once unij_once_t;

/**
 * Run once callback declaration
 * @typedef PRUN_ONCE_FN
 * @param[in]
 */
typedef BOOL(CDECL *unij_once_fn)(void* parameter);

/**
 * @def unij_once_init
 * Static initializer for one-time initialization structure.
 */
#define UNIJ_ONCE_INIT {0}

/**
 * Essentially InitOnceExecuteOnce for pre-Vista Windows without the context it.
 * @param[in,out] once Pointer to the one-time initialiation structure.
 * @param[in] oncefn Application-defined one-time callback.
 * @param[in,out] parameter Application-defined data passed into the callback
 * @return Success of the callback.
 */
bool unij_once(unij_once_t* once, unij_once_fn oncefn, void* parameter);

/**
 * Adjust a process's token privileges, enabling all privileges named in the
 * pNames parameter.
 * @param[in] process Process handle to target
 * @param[in] names An array of privilege names
 * @param[in] dwCount Number of privileges specified
 * @return Success status
 */
bool unij_acquire_privileges(HANDLE process, const wchar_t** names, uint32_t count);

/**
 * Adjust current process's token privileges, enabling a preset list.
 * @return Success status
 */
bool unij_acquire_default_privileges(void);

/**
 * @brief Creates a win32 object name based on \a UNIJ_OBJECT_FORMAT
 * @param[in] key 
 * @param[in] type 
 * @param[in] pid 
 * @return 
 */
const wchar_t* unij_object_name(const wchar_t* key, unij_object_t type, uint32_t pid);

/**
 * @brief 
 * @param[in] name 
 * @param[in] size 
 * @return 
 */
HANDLE unij_create_mmap(const wchar_t* name, size_t size);

/**
 * @brief 
 * @param[in] name 
 * @param[in] readonly 
 * @return 
 */
HANDLE unij_open_mmap(const wchar_t* name, bool readonly);

/**
 * @brief 
 * @param[out] pname 
 * @param[in] key 
 * @param[in] pid 
 * @return 
 */
HANDLE unij_create_event(const wchar_t* name);

#ifdef __cplusplus
}
#endif

#endif /* _UNIJECT_WIN32_H_ */
