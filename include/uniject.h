/**
 * @file uniject.h
 * 
 * TODO: Description
 */
#ifndef _UNIJECT_H_
#define _UNIJECT_H_
#pragma once

#include "uniject/preconfig.h"
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <limits.h>
#include <wchar.h>

#if defined(UNIJ_CC_MSVC) && (_MSC_VER < 1600)

typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

typedef __int8 _Bool;
#	define bool _Bool
#	define false 0
#	define true 1
#	define __bool_true_false_are_defined 1
#else
#	include <stdint.h>
#	include <stdbool.h>
#endif

#ifndef CDECL
#	define CDECL __cdecl
#endif

#ifndef UNIJ_EXTERN
#	define UNIJ_EXTERN extern
#endif

// For functions meant to be inlined
#if defined(__always_inline)
#	define UNIJ_INLINE __always_inline
#elif defined(UNIJ_CC_MSVC)
#	define UNIJ_INLINE __forceinline
#elif (defined(UNIJ_CC_GNU) || defined(UNIJ_CC_CLANG))
#	define UNIJ_INLINE inline __attribute__(( __always_inline__ ))
#else
#	define UNIJ_INLINE 
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

// UNIJ_NORETURN
#if defined(UNIJ_CC_MSVC)
#	define UNIJ_NORETURN __declspec(noreturn) void
#else
#	define UNIJ_NORETURN __attribute__((noreturn)) void
#endif

/* Macro param helper */
#define UNIJ_SINGLE(...) __VA_ARGS__

/* Token pasting macro */
#define _UNIJ_PASTE2(A,B) A##B
#define UNIJ_PASTE(A,B) _UNIJ_PASTE2(A,B)

/* Stringifying macros */
#define _UNIJ_STRINGIFY2(X) #X
#define UNIJ_WIDEN(X) UNIJ_PASTE(L,X)
#define UNIJ_STRINGIFY(...) _UNIJ_STRINGIFY2( __VA_ARGS__ )
#define UNIJ_WSTRINGIFY(...) UNIJ_WIDEN(UNIJ_STRINGIFY( __VA_ARGS__ ))

/* Compiler messages */
#if defined(UNIJ_CC_MSVC)
#	define UNIJ_MESSAGE(M) \
	__pragma(message( "[" __FILE__ "(" UNIJ_STRINGIFY(__LINE__) ")] MESSAGE: " M ))
#else
#	define UNIJ_MESSAGE(M) \
	__Pragma(message( "[" __FILE__ "(" UNIJ_STRINGIFY(__LINE__) ")][MESSAGE][" __TIMESTAMP__ "]: " M ))
#endif

/* Alignment - parameterizing due to the incompatibility in ordering between compilers */
#if defined(UNIJ_CC_MSVC)
#	define UNIJ_ALIGN(N) __declspec(align(N))
#else
#	define UNIJ_ALIGN(N) __attribute__(( aligned(N) ))
#endif

/* Using the cache alignment that's in line with most commodity processors */
#define UNIJ_CACHE_ALIGN UNIJ_ALIGN(64)

/**
 * @def UNIJ_UNUSED(X)
 * @brief Suppressed unused parameter warnings (hopefully)
 */
#define UNIJ_SUPPRESS_UNUSED(X) \
	((void)(X))

/**
 * @def UNIJ_DLLIMP
 * @brief DLL import macro.
 * TODO: figure out what to use in clang and mingw
 */
#define UNIJ_DLLIMP __declspec(dllimport)

/**
 * @def UNIJ_DLLEXP
 * @brief DLL export macro.
 * TODO: figure out what to use in clang and mingw
 */
#define UNIJ_DLLEXP __declspec(dllexport)

typedef struct unij_wstr unij_wstr_t;
typedef struct unij_cstr unij_cstr_t;

/**
 * @typedef unij_wstr_t
 * @brief Wide string holder
 * \a value may or may not contain a trailing zero. If \a length is 0, \a value should be NULL. Empty
 * strings should not be persisted.
 */
struct unij_wstr
{
	uint16_t length;
	const wchar_t* value;
};

#define UNIJ_EMPTY_WSTR ((unij_wstr_t){ 0, NULL })

/**
 * @typedef unij_cstr_t
 * @brief String holder
 * \a value may or may not contain a trailing zero. If \a length is 0, \a value should be NULL. Empty
 * strings should not be persisted.
 */
struct unij_cstr
{
	uint16_t length;
	const char* value;
};

#define UNIJ_EMPTY_CSTR ((unij_cstr_t){ 0, NULL })

// Empty checker for unij_wstr_t's and unij_cstr_t's. `_` prefix as it will be wrapped in a macro that
// will allow it to also support unij_cstr_t
static UNIJ_INLINE bool _unij_is_empty(const unij_wstr_t* value)
{
	return (value == NULL) || (value->length == 0) || (value->value == NULL);
}

#define unij_is_empty(VALUE) \
	(_unij_is_empty( (const unij_wstr_t*)(VALUE) ))

#ifdef __cplusplus
}
#endif

#include "uniject/crt.h"

#endif /* _UNIJECT_H_ */
