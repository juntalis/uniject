/**
 * @file uniject/injector.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Injector-specific functionality
 */
#ifndef _UNIJECT_INJECTOR_H_
#define _UNIJECT_INJECTOR_H_
#pragma once

#include <uniject.h>
#include <uniject/error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct unij_process unij_process_t;
typedef void(CDECL* unij_hijack_fn)(void* param);

bool unij_hijack_thread(HANDLE thread, unij_hijack_fn fn, void* param);
bool unij_hijack_thread_id(uint32_t tid, unij_hijack_fn fn, void* param);
bool unij_inject_loader_ex(unij_process_t* process, unij_wstr_t* loader);

static UNIJ_INLINE bool unij_inject_loader(unij_process_t* process)
{
	return unij_inject_loader_ex(process, NULL);
}

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_INJECTOR_H_ */
