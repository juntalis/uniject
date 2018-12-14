/**
 * @file pch.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for pch.h
 */
#if !defined(_CLI_PCH_H_) && !defined(_PARG_C_)
#define _CLI_PCH_H_
#pragma once

#define UNIJ_UTILITY_MACROS 1

#include <uniject.h>
#include <uniject/base.h>
#include <uniject/error.h>
#include <uniject/utility.h>
#include <uniject/win32.h>

#include <limits.h>

// length stuff
#define ARRAYLEN(ARRAY) \
	(sizeof(ARRAY) / sizeof(*(ARRAY)))

#define STRINGLEN(STRING) \
	(ARRAYLEN(STRING) - 1)

#define DEFINE_STATIC_WSTR(NAME,TEXT) \
	static wchar_t UNIJ_PASTE(NAME,_TEXT) [] = TEXT ; \
	static unij_wstr_t NAME = { STRINGLEN(TEXT), UNIJ_PASTE(NAME,_TEXT) }

#define DEFINE_STATIC_CSTR(NAME,TEXT) \
	static char UNIJ_PASTE(NAME,_TEXT) [] = TEXT ; \
	static unij_cstr_t NAME = { STRINGLEN(TEXT), UNIJ_PASTE(NAME,_TEXT) }

// String Validation
#define IS_VALID_STRING(STR) \
	(((STR) != NULL) && (*(STR) != 0))

#define IS_INVALID_STRING(STR) \
	(!IS_VALID_STRING(STR))

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

#endif /* _CLI_PCH_H_ */
