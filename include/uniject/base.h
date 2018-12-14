/**
 * @file uniject/base.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Contains the top-level declarations for using this library.
 */
#ifndef _UNIJECT_BASE_H_
#define _UNIJECT_BASE_H_
#pragma once

#include <uniject.h>
#include <uniject/error.h>
#include <uniject/params.h>

#ifdef __cplusplus
extern "C" {
#endif

bool unij_init(void);

typedef struct uniject uniject_t;

uniject_t* unij_loader_open(void);
uniject_t* unij_injector_open(uint32_t pid);
uniject_t* unij_injector_assign(HANDLE process_handle);
uniject_t* unij_injector_open_params(const unij_params_t* params);

// Injector REQUIRES a call to \a unij_set_assembly_path before calling \a unij_inject
void unij_set_assembly_path(uniject_t* ctx, unij_wstr_t* path);

bool unij_inject(uniject_t* ctx);

// Returns a pointer to the actual params struct. Quicker than the individual unij_set_* calls, but you give up
// the minimal safety checks in those functions.
//
// NOTE: \a unij_wstr_t fields are expected to be { 0, NULL } if empty. If not empty, they're expected to contain
//       value pointers allocated with \a unij_alloc. \a unij_close will attempt to free any non-NULL values.
unij_params_t* unij_get_params(uniject_t* ctx);

uint32_t unij_get_pid(uniject_t* ctx);
uint32_t unij_get_tid(uniject_t* ctx);
bool unij_get_debugging(uniject_t* ctx);

// wstr params return pointers to the actual param field. It's assumed that the user won't be dumb and free or
// modify them unnecessarily
unij_wstr_t* unij_get_mono_path(uniject_t* ctx);
unij_wstr_t* unij_get_assembly_path(uniject_t* ctx);
unij_wstr_t* unij_get_class_name(uniject_t* ctx);
unij_wstr_t* unij_get_method_name(uniject_t* ctx);
unij_wstr_t* unij_get_log_path(uniject_t* ctx);

void unij_set_tid(uniject_t* ctx, uint32_t tid);
void unij_set_debugging(uniject_t* ctx, bool enabled);
//void unij_set_mono_path(uniject_t* ctx, unij_wstr_t* path);
void unij_set_class_name(uniject_t* ctx, unij_wstr_t* path);
void unij_set_log_path(uniject_t* ctx, unij_wstr_t* path);
void unij_set_loader_path(uniject_t* ctx, unij_wstr_t* path);

void unij_close(uniject_t* ctx);

/**
 * @brief Future site of the user-overridable handlers declaration.
 * TODO: Allow overriding various handlers in one go.
 */
struct unij_handlers
{
	/**
	 * @brief Anything stored in the \a context field will be passed in as the first arguments for each function.
	 */
	void* context;
	
	/**
	 * @brief Should point to an application callback for handling memory allocations.
	 * @param[in] context Context context - read from the field above.
	 * @param[in] size The requested size of the allocation. (byte count)
	 * @return A newly allocated buffer or NULL in the case of failure.
	 */
	void*(CDECL* alloc_fn)(void* context, size_t size);
	
	/**
	 * @brief Should point to an application callback for handling the cleanup of \a alloc_fn.
	 * @param[in] context Context context - read from the field above.
	 * @param[in] ptr The allocated memory we're looking to free.
	 * @param[in] size The size of our allocation
	 */
	void(CDECL* free_fn)(void* context, void* ptr);
	
	/**
	 * @brief Should point to an application callback for handling errors.
	 * @param[in] context Context context - read from the field above.
	 * @param[in] code Relevant error code
	 * @param[in] message Relevant error message
	 */
	void(CDECL* error_fn)(void* context, unij_error_t code, const wchar_t* message);
};

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_BASE_H_ */
