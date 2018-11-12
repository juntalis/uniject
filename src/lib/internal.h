/**
 * @file internal.h
 * 
 * TODO: Description
 */
#ifndef _UNIJ_INTERNAL_H_
#define _UNIJ_INTERNAL_H_
#pragma once

#if (!defined(_LIBUNIJECT_H_) || !defined(UNIJ_BUILD))
#	error internal.h should only be included by pch.h during a build of the library.
#endif

#ifndef _UNIJECT_BUILD_CONFIG_H_
#	include "build_config.h"
#endif

// For functions that should not be inlined
#if defined(__attribute_noinline__)
#	define UNIJ_NOINLINE __attribute_noinline__
#elif defined(UNIJ_CC_MSVC)
#	define UNIJ_NOINLINE __declspec(noinline)
#elif (defined(UNIJ_CC_GNU) || defined(UNIJ_CC_CLANG))
#	define UNIJ_NOINLINE __attribute__(( __noinline__ ))
#else
#	define UNIJ_NOINLINE 
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
#define UNIJ_OBJECT_FORMAT \
	UNIJ_WIDEN(UNIJ_OBJECT_SCOPE) L"\\" UNIJ_WIDEN(UNIJ_OBJECT_PREFIX) L".%s:%s@%08X"

// Wide stringify the params key
#define UNIJ_PARAMS_KEYW \
	UNIJ_WIDEN(UNIJ_PARAMS_KEY)

/* Format Strings */

#define UNIJ_FORMAT_U8  L"%" UNIJ_WIDEN(PRIu8)
#define UNIJ_FORMAT_U16 L"%" UNIJ_WIDEN(PRIu16)
#define UNIJ_FORMAT_U32 L"%" UNIJ_WIDEN(PRIu32)
#define UNIJ_FORMAT_U64 L"%" UNIJ_WIDEN(PRIu64)

// Replacements for C runtime functions.
#ifdef UNIJ_CC_MSVC

#ifdef __cplusplus
extern "C" {
#endif

#	undef RtlFillMemory
#	undef RtlZeroMemory
#	undef RtlMoveMemory
#	undef RtlCopyMemory
#	undef RtlEqualMemory
#	undef RtlCompareMemory
typedef ULONG LOGICAL;
UNIJ_DLLIMP void WINAPI RtlFillMemory(PVOID, SIZE_T, BYTE);
UNIJ_DLLIMP void WINAPI RtlZeroMemory(PVOID, SIZE_T);
UNIJ_DLLIMP void WINAPI RtlMoveMemory(PVOID, const VOID*, SIZE_T);
UNIJ_DLLIMP void WINAPI RtlCopyMemory(PVOID, const VOID*, SIZE_T);
UNIJ_DLLIMP LOGICAL WINAPI RtlEqualMemory(const VOID*, const VOID*, SIZE_T);
UNIJ_DLLIMP SIZE_T WINAPI RtlCompareMemory(const VOID*, const VOID*, SIZE_T);

#ifdef __cplusplus
}
#endif

#endif

#endif /* _UNIJ_INTERNAL_H_ */
