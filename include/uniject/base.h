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

#ifdef __cplusplus
extern "C" {
#endif

bool unij_init(void);
bool unij_shutdown(void);

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
