/**
 * @file error.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "error_private.h"
#include "uniject/logger.h"
#include "uniject/utility.h"

static UNIJ_INLINE const wchar_t* get_last_error_description(uint32_t last_error)
{
	const wchar_t* message = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, last_error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(wchar_t*)&message, 0, NULL
	);
	return message;
}

const wchar_t* unij_get_error_text(unij_error_t code)
{
	uint32_t last_error = GetLastError();
	
	switch(code)
	{
		case UNIJ_ERROR_PID:
			return L"Invalid/non-existent PID or TID specified";
		case UNIJ_ERROR_PROCESS:
			return L"Target process is not supported (Likely not a Unity game)";
		case UNIJ_ERROR_LOADERS:
			return L"Could not locate the necessary loader DLL file";
		case UNIJ_ERROR_PARAM:
			return L"Invalid parameter specified";
		case UNIJ_ERROR_MONO:
			return L"Loaders couldn't locate the mono module";
		case UNIJ_ERROR_ASSEMBLY:
			return L"Loaders couldn't locate the target assembly";
		case UNIJ_ERROR_METHOD:
			return L"Loaders couldn't resolve the target method to call";
		case UNIJ_ERROR_INTERNAL:
			return L"Internal error occurred within uniject code";
		case UNIJ_ERROR_OPERATION:
			return L"Incorrect usage of uniject API from the calling code";
		case UNIJ_ERROR_SUCCESS:
		case UNIJ_ERROR_ADDRESS:
		case UNIJ_ERROR_OUTOFMEMORY:
		case UNIJ_ERROR_PATHNAME:
			last_error = (uint32_t)code;
		case UNIJ_ERROR_LASTERROR:
			return get_last_error_description(last_error);
	}
	
	return L"Unknown Error";
}

void unij_free_error_text(unij_error_t code, const wchar_t* message)
{
	switch(code)
	{
		case UNIJ_ERROR_SUCCESS:
		case UNIJ_ERROR_ADDRESS:
		case UNIJ_ERROR_OUTOFMEMORY:
		case UNIJ_ERROR_PATHNAME:
		case UNIJ_ERROR_LASTERROR:
			LocalFree((HLOCAL)message);
			break;
		default:
			break;
	}
}

static UNIJ_INLINE void vshow_message(unij_level_t level, const wchar_t* format, va_list args)
{
	const wchar_t* message = unij_vsawprintf(format, args);
	unij_show_message_impl(level, message);
	if(level >= UNIJ_LEVEL_ERROR) {
		LogError(message);
	}
	unij_free((void*)message);
}

void unij_show_message(unij_level_t level, const wchar_t* format, ...)
{
	va_list vargs;
	va_start(vargs, format);
	vshow_message(level, format, vargs);
	va_end(vargs);
}

void unij_fatal_error(unij_error_t code, const wchar_t* format, ...)
{
	va_list vargs;
	const wchar_t* desc = NULL;
	
	// Store last error for potential future use.
	uint32_t last_error = GetLastError();
	desc = unij_get_error_text(code);
	
	// Determine what to do with format
	if(IS_INVALID_STRING(format)) {
		unij_show_message_impl(UNIJ_LEVEL_FATAL, desc);
	} else {
		// Build new format string
		const wchar_t* new_format = unij_sawprintf(L"%s - %s", desc, format);
		ASSERT_VALID_STRING(new_format);
		
		// Apply new format string.
		va_start(vargs, format);
		vshow_message(UNIJ_LEVEL_FATAL, new_format, vargs);
		va_end(vargs);
		
		// Cleanup
		unij_free((void*)new_format);
	}
	
	// Cleanup error description
	unij_free_error_text(code, desc);
	
	// Restore LastError
	SetLastError(last_error);
	
	// Trigger the abort handler.
	unij_abort(code);
}

// Stupid fucking windows headers
#ifdef ERROR
#	undef ERROR
#endif

#define DEFINE_STATIC_WSTR(NAME,TEXT) \
	static wchar_t UNIJ_PASTE(NAME,_text) [] = TEXT ; \
	static unij_wstr_t NAME = { STRINGLEN(TEXT), UNIJ_PASTE(NAME,_text) }

#define LEVEL_CASE(LEVEL) \
	case UNIJ_PASTE(UNIJ_LEVEL_,LEVEL): { \
		DEFINE_STATIC_WSTR( LEVEL##_LABEL , UNIJ_WSTRINGIFY(LEVEL) ); \
		return &(LEVEL##_LABEL); \
	}

const unij_wstr_t* unij_level_name(unij_level_t level)
{
	switch(level)
	{
		LEVEL_CASE(INFO);
		LEVEL_CASE(WARNING);
		LEVEL_CASE(ERROR);
		LEVEL_CASE(FATAL);
		default: {
			DEFINE_STATIC_WSTR(unknown_label, L"UNKNOWN");
			return &unknown_label;
		}
	}
}

