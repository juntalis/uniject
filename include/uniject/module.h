/**
 * @file uniject/module.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for module.h
 */
#ifndef _UNIJECT_MODULE_H_
#define _UNIJECT_MODULE_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves the handle for the named module (in the current process) without
 * incrementing the module's reference count.
 * @param[in] pModuleName Module name
 * @param[in] dwFlags Any additional flags
 * @return Module handle or NULL if the module was not found.
 */
HMODULE unij_noref_module_ex(const wchar_t* pModuleName, uint32_t dwFlags);

/**
 * ::unij_noref_module_ex without the flags param.
 * @param[in] pModuleName Module name
 * @return Module handle or NULL if the module was not found.
 */
static UNIJ_INLINE HMODULE unij_noref_module(const wchar_t* pModuleName)
{
	return unij_noref_module_ex(pModuleName, 0);
}

/**
 * ::GetModuleNoRef, defaulting to the current module.
 * @return Module handle or NULL if the module was not found. (should NOT happen)
 */
static UNIJ_INLINE HMODULE unij_current_module(void)
{
#	ifndef GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
#		define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 0x00000004
#	endif
	return unij_noref_module_ex((const wchar_t*)&unij_noref_module_ex, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS);
}

/**
 * @brief 
 * @param buffer 
 * @param psize 
 * @param module 
 * @param bits 
 * @return 
 */
bool unij_system_module_path(wchar_t* buffer, uint32_t* psize, const wchar_t* module, int bits);

/**
 * @brief 
 * @param module 
 * @param proc 
 * @return 
 */
uint32_t unij_get_proc_rva(const wchar_t* module, const char* proc);

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_MODULE_H_ */
