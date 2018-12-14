/**
 * @file pch.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for pch.h
 */
#ifndef _DLL_PCH_
#define _DLL_PCH_
#pragma once

#define UNIJ_UTILITY_MACROS 1

#include <uniject.h>
#include <uniject/base.h>
#include <uniject/error.h>
#include <uniject/utility.h>
#include <uniject/win32.h>

// length stuff
#define ARRAYLEN(ARRAY) \
	(sizeof(ARRAY) / sizeof(*(ARRAY)))

#define STRINGLEN(STRING) \
	(ARRAYLEN(STRING) - 1)

#define DEFINE_STATIC_CSTR(NAME,TEXT) \
	static char UNIJ_PASTE(NAME,_TEXT) [] = TEXT ; \
	static unij_cstr_t NAME = { STRINGLEN(TEXT), UNIJ_PASTE(NAME,_TEXT) }

#define DEFINE_STATIC_WSTR(NAME,TEXT) \
	static wchar_t UNIJ_PASTE(NAME,_TEXT) [] = TEXT ; \
	static unij_wstr_t NAME = { STRINGLEN(TEXT), UNIJ_PASTE(NAME,_TEXT) }

#endif /* _DLL_PCH_ */
