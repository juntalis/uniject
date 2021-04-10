/**
 * @file internal.h
 * 
 * TODO: Description
 */
#ifndef _INTERNAL_H_
#define _INTERNAL_H_
#pragma once

#ifndef _UNIJECT_H_
#	include <uniject.h>
#endif

#ifndef _UNIJECT_BUILD_CONFIG_H_
#	include "build_config.h"
#endif

//	Fix params mismatch between swprintf and sprintf
#if defined(UNIJ_CC_MSVC) && !defined(_CRT_NON_CONFORMING_SWPRINTFS)
#	define _CRT_NON_CONFORMING_SWPRINTFS 1
#endif

// Normalization
#ifndef MAX_PATH
#	if defined(_MAX_PATH)
#		define MAX_PATH _MAX_PATH
#	elif defined(PATH_MAX)
#		define MAX_PATH PATH_MAX
#	else
#		define MAX_PATH 260
#	endif
#endif

/* Constants */

// Limits
#ifndef MAXULONGLONG
#	define MAXULONGLONG ((uint64_t)~((uint64_t)0))
#endif

// Empty strings
#ifndef EMPTY_STRINGA
#	define EMPTY_STRINGA ""
#endif

#ifndef EMPTY_STRINGW
#	define EMPTY_STRINGW L""
#endif

/* Utility macros */

// Make a uint64_t from the corresponding low/high words
#ifndef MAKEULONGLONG
#	define MAKEULONGLONG(LOWDWORD, HIGHDWORD) (((uint64_t)(HIGHDWORD) << 32) | ((LOWDWORD) & 0xFFFFFFFF))
#endif

#define STATIC_ASSERT2(COND,MSG) \
	static int UNIJ_PASTE(STATIC_ASSERTION_,MSG)[(COND)?1:-1]

#define STATIC_ASSERT(COND) \
	STATIC_ASSERT2(COND,__COUNTER__)

// Numeric validation
#define ASSERT_NOT_ZERO(VAL) \
	assert( (VAL) != 0 )

// Pointer validation
#define ASSERT_NOT_NULL(PTR) \
	assert( (PTR) != NULL )

// String Validation
#define IS_VALID_STRING(STR) \
	(((STR) != NULL) && (*(STR) != 0))

#define IS_INVALID_STRING(STR) \
	(!IS_VALID_STRING(STR))

#define IS_EMPTY_STRING(STR) \
	(((STR) != NULL) && (*(STR) == 0))

#define ASSERT_VALID_STRING(STR) \
	assert( IS_VALID_STRING(STR) )

// Handle Validation
#define IS_VALID_HANDLE(SUBJECT) \
	(((SUBJECT) != NULL) && ((SUBJECT) != INVALID_HANDLE_VALUE))

#define IS_INVALID_HANDLE(SUBJECT) \
	(((SUBJECT) == NULL) || ((SUBJECT) == INVALID_HANDLE_VALUE))

#define ASSERT_VALID_HANDLE(SUBJECT) \
	assert( IS_VALID_HANDLE(SUBJECT) )

// length stuff
#define ARRAYLEN(ARRAY) \
	(sizeof(ARRAY) / sizeof(*(ARRAY)))

#define STRINGLEN(STRING) \
	(ARRAYLEN(STRING) - 1)

#define WSIZE(COUNT) \
	((size_t)((COUNT) * sizeof(wchar_t)))

/* Kernel Object */

// Resulting format string for object names based on the values of 
// UNIJ_OBJECT_SCOPE and UNIJ_OBJECT_PREFIX.
// TODO: pid is coming up as 0 in the loader dll call. Need to find out why
#define UNIJ_OBJECT_FORMAT \
	UNIJ_WIDEN(UNIJ_OBJECT_SCOPE) L"\\" UNIJ_WIDEN(UNIJ_OBJECT_PREFIX) L".%s:%s"
//	UNIJ_WIDEN(UNIJ_OBJECT_SCOPE) L"\\" UNIJ_WIDEN(UNIJ_OBJECT_PREFIX) L".%s:%s@%08X"

// Wide stringify the params key
#define UNIJ_PARAMS_KEYW \
	UNIJ_WIDEN(UNIJ_PARAMS_KEY)

// Wide stringify the loader basename
#define UNIJ_LOADER_BASENAMEW \
	UNIJ_WIDEN(UNIJ_LOADER_BASENAME)

// Wide stringify the loader basename
#define UNIJ_LOADER32_NAME \
	UNIJ_LOADER_BASENAMEW L"-32.dll"

// Wide stringify the loader basename
#define UNIJ_LOADER64_NAME \
	UNIJ_LOADER_BASENAMEW L"-64.dll"

#define MAKE_PTR(TYPE,BASE,OFFSET) \
	( (TYPE*)(AS_UPTR(BASE) + AS_UPTR(OFFSET)) )

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internally used helper for normalizing the process of getting a wstring's length without doing checks.
 * @param[in] str Pointer to test 
 * @return `true` if NULL or empty. `false` otherwise.
 */
static UNIJ_INLINE uint16_t unij_wstrlen(const unij_wstr_t* str)
{
	uint16_t length = 0;
	if(str != NULL && IS_VALID_STRING(str->value)) {
		length = str->length == 0 ? (uint16_t)lstrlenW(str->value) : str->length;
	}
	return length;
}

/**
 * @brief Interally-used for normalizing a \a unij_wstr_t struct after already verifying a non-NULL pointer.
 * @param[in,out] str Pointer to the \a unij_wstr_t we're operating on. 
 * @return `false` if empty. `true` otherwise. \a str is altered with the correct length if length came in as 0.
 */
static UNIJ_INLINE bool unij_wstring(unij_wstr_t* str)
{
	if(str == NULL) {
		return false;
	} else {
		str->length = unij_wstrlen(str);
	}
	return str->length > 0;
}

#ifdef __cplusplus
}
#endif

#include "error_private.h"

#define UNIJ_SHOW_INFO(...) \
	unij_show_message(UNIJ_LEVEL_INFO, __VA_ARGS__ )

#endif /* _INTERNAL_H_ */
