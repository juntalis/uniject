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
const wchar_t* unij_prefix_vsawprintf(const wchar_t* prefix, const wchar_t* format, va_list args);
const wchar_t* unij_prefix_sawprintf(const wchar_t* prefix, const wchar_t* format, ...);

#define unij_vsawprintf(format,args) (unij_prefix_vsawprintf(NULL, format, args))
#define unij_sawprintf(...) (unij_prefix_sawprintf(NULL, __VA_ARGS__ ))

wchar_t* unij_simplify_path(wchar_t* dest, const wchar_t* src, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* _UNIJECT_UTILITY_H_ */
