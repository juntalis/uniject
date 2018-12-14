/**
 * @file uniject/utility.h
 * 
 * Miscellaneous functionality shared by the injector and loaders.
 */
#ifndef _UNIJECT_UTILITY_H_
#define _UNIJECT_UTILITY_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(WSIZE) && defined(UNIJ_UTILITY_MACROS)
#	define WSIZE(COUNT) \
		((size_t)((COUNT) * sizeof(wchar_t)))

// Make a uint64_t from the corresponding low/high words
#	ifndef MAKEULONGLONG
#		define MAKEULONGLONG(LOWDWORD, HIGHDWORD) (((uint64_t)(HIGHDWORD) << 32) | ((LOWDWORD) & 0xFFFFFFFF))
#	endif
#endif

void* unij_alloc(size_t size);
void unij_free(void* ptr);

/**
 * @brief Wrapper around \a unij_alloc for wide character strings.
 * Doesn't assume the caller wants an additional byte for the trailing zero. If you need one, add it yourself.
 * @param[in] length Length of allocation in wide characters
 * @return Allocated string or NULL
 */
static UNIJ_INLINE wchar_t* unij_wcsalloc(size_t length)
{
	return unij_alloc(length * sizeof(wchar_t));
}

// TODO: Should probably expose the resulting length to the caller.
wchar_t* unij_wcsdup(const wchar_t* src);
wchar_t* unij_wcsndup(const wchar_t* src, size_t count);

// TODO: unij_wstrndup
unij_wstr_t unij_wstrdup(const unij_wstr_t* str);

const wchar_t* unij_prefix_vsawprintf(const wchar_t* prefix, const wchar_t* format, va_list args);
const wchar_t* unij_prefix_sawprintf(const wchar_t* prefix, const wchar_t* format, ...);

#define unij_vsawprintf(format,args) (unij_prefix_vsawprintf(NULL, format, args))
#define unij_sawprintf(...) (unij_prefix_sawprintf(NULL, __VA_ARGS__ ))

const char* unij_prefix_vsacprintf(const char* prefix, const char* format, va_list args);
const char* unij_prefix_sacprintf(const char* prefix, const char* format, ...);

#define unij_vsacprintf(format,args) (unij_prefix_vsacprintf(NULL, format, args))
#define unij_sacprintf(...) (unij_prefix_sacprintf(NULL, __VA_ARGS__ ))

unij_cstr_t unij_wstrtocstr(const unij_wstr_t* str);

/**
 * @brief Not safe to use for your general lowercasing needs. Specific to the data we intend to process.
 * @param[in,out] buffer - Buffer to operate on in place 
 * @param[in] length - Length of buffer 
 * @return \a buffer
 */
wchar_t* unij_strtolower(wchar_t* buffer, size_t length);

#define unij_cstrfree(STR) \
	unij_wstrfree((unij_wstr_t*)STR)

static UNIJ_INLINE void unij_wstrfree(unij_wstr_t* str)
{
	if(str != NULL && str->value != NULL) {
		unij_free((void*)str->value);
		RtlZeroMemory((void*)str, sizeof(*str));
	}
}

#ifdef __cplusplus
}
#endif

#endif /* _UNIJECT_UTILITY_H_ */
