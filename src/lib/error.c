/**
 * @file error.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "uniject/error.h"
#include "uniject/logger.h"
#include "uniject/utility.h"

static UNIJ_INLINE const wchar_t* GetLastErrorDescription(uint32_t dwError)
{
	const wchar_t* pMessage = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(wchar_t*)&pMessage, 0, NULL
	);
	return pMessage;
}

const wchar_t* unij_get_error_text(unij_error_t uCode)
{
	uint32_t dwError = GetLastError();
	
	switch(uCode)
	{
		case UNIJ_ERROR_PID:
			return L"Invalid/non-existent PID specified";
		case UNIJ_ERROR_PROCESS:
			return L"Target process is not supported (Likely not a Unity game)";
		case UNIJ_ERROR_LOADERS:
			return L"Could not locate the necessary loader DLL file";
		case UNIJ_ERROR_UNPACK:
			return L"An error occurred while attempting to unpack loader params";
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
			dwError = (uint32_t)uCode;
		case UNIJ_ERROR_LASTERROR:
			return GetLastErrorDescription(dwError);
	}
	
	return L"Unknown Error";
}

void unij_free_error_text(unij_error_t uCode, const wchar_t* pMessage)
{
	switch(uCode)
	{
		case UNIJ_ERROR_SUCCESS:
		case UNIJ_ERROR_ADDRESS:
		case UNIJ_ERROR_OUTOFMEMORY:
		case UNIJ_ERROR_PATHNAME:
		case UNIJ_ERROR_LASTERROR:
			LocalFree((HLOCAL)pMessage);
			break;
		default:
			break;
	}
}

 UNIJ_INLINE void vshow_message(unij_level_t uLevel, const wchar_t* pFormat, va_list vArgs)
{
	const wchar_t* pMessage = unij_vsawprintf(pFormat, vArgs);
	unij_show_message_impl(uLevel, pMessage);
	if(uLevel >= UNIJ_LEVEL_ERROR) {
		LogError(pMessage);
	}
	unij_free((void*)pMessage);
}

void unij_show_message(unij_level_t uLevel, const wchar_t* pFormat, ...)
{
	va_list vArgs;
	va_start(vArgs, pFormat);
	vshow_message(uLevel, pFormat, vArgs);
	va_end(vArgs);
}

void unij_fatal_error(unij_error_t uCode, const wchar_t* pFormat, ...)
{
	va_list vArgs;
	const wchar_t* pDesc = NULL;
	
	// Store last error for potential future use.
	uint32_t dwError = GetLastError();
	pDesc = unij_get_error_text(uCode);
	
	// Determine what to do with pFormat
	if(IS_INVALID_STRING(pFormat)) {
		unij_show_message_impl(UNIJ_LEVEL_FATAL, pDesc);
	} else {
		// Build new format string
		const wchar_t* pNewFormat = unij_sawprintf(L"%s [%s]", pFormat, pDesc);
		ASSERT_VALID_STRING(pNewFormat);
		
		// Apply new format string.
		va_start(vArgs, pFormat);
		vshow_message(UNIJ_LEVEL_FATAL, pNewFormat, vArgs);
		va_end(vArgs);
		
		// Cleanup
		unij_free((void*)pNewFormat);
	}
	
	// Cleanup error description
	unij_free_error_text(uCode, pDesc);
	
	// Restore LastError
	SetLastError(dwError);
	
	// Trigger the abort handler.
	unij_abort(uCode);
}


