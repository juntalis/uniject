/**
 * @file uniject/error.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Took a wrong turn with this and need to eventually change it back to my original idea.
 */
#ifndef _UNIJECT_ERROR_H_
#define _UNIJECT_ERROR_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Describes the severity of the message that accompanies it. (intended for accurate visual cues) 
 */
enum unij_level
{
	UNIJ_LEVEL_INFO = 0,
	UNIJ_LEVEL_WARNING,
	UNIJ_LEVEL_ERROR,
	UNIJ_LEVEL_FATAL
};

/**
 * Error codes used 
 */
enum unij_error
{
	/**
	 * @brief Success - not an error. 
	 */
	UNIJ_ERROR_SUCCESS = ERROR_SUCCESS,
	
	/**
	 * @brief Attempt to access invalid address.
	 */
	UNIJ_ERROR_ADDRESS = ERROR_INVALID_ADDRESS,
	
	/**
	 * @brief Invalid parameter specified.
	 */
	UNIJ_ERROR_PARAM = ERROR_INVALID_PARAMETER,
	
	/**
	 * @brief Out of memory.exception.
	 */
	UNIJ_ERROR_OUTOFMEMORY = ERROR_OUTOFMEMORY,
	
	/**
	 * @brief Used if an internal error in uniject is detected. See message for details.
	 */
	UNIJ_ERROR_INTERNAL = ERROR_INTERNAL_ERROR,
	
	/**
	 * @brief Incorrect usage of uniject API by the calling code. See message for details.
	 */
	UNIJ_ERROR_OPERATION = ERROR_INVALID_OPERATION,
	
	/**
	 * @brief Invalid/non-existent PID or TID specified.
	 */
	UNIJ_ERROR_PID =  ERROR_INVALID_DATA,
	
	/**
	 * @brief Target process is not supported. (Likely not a Unity game)
	 */
	UNIJ_ERROR_PROCESS = ERROR_BAD_EXE_FORMAT,
	
	/**
	 * @brief Could not locate the necessary loader DLL file.
	 */
	UNIJ_ERROR_LOADERS = ERROR_FILE_NOT_FOUND,
	
	/**
	 * @brief Loaders couldn't locate the mono module.
	 */
	UNIJ_ERROR_MONO = ERROR_MOD_NOT_FOUND,
	
	/**
	 * @brief The specified path is invalid.
	 */
	UNIJ_ERROR_PATHNAME = ERROR_BAD_PATHNAME,
	
	/**
	 * @brief Loaders couldn't locate the target assembly.
	 */
	UNIJ_ERROR_ASSEMBLY = ERROR_PATH_NOT_FOUND,
	
	/**
	 * @brief Loaders couldn't resolve the target method to call.
	 */
	UNIJ_ERROR_METHOD = ERROR_PROC_NOT_FOUND,
	
	/**
	 * @brief Win32 System Error
	 * Indicates that a Win32 API call failed, and that the system
	 * error code can be retrieved with a call to GetLastError.
	 */
	UNIJ_ERROR_LASTERROR = -1
};

typedef enum unij_error unij_error_t;

typedef enum unij_level unij_level_t;

/**
 * @def unij_trace_prefix(TEXT)
 * @brief Helper macro for prefixing some static text with the current file and line number.
 */
#define UNIJ_TRACE_PREFIX(TEXT) \
	L"[" UNIJ_WIDEN(__FILE__) L":" UNIJ_WSTRINGIFY(__LINE__) L"] " TEXT

/**
 * @brief Application-specific display of messages to user.
 * In order to give the application/DLL full control over the display of messages and critical failure process, we leave
 * this and ::unij_abort_impl as undefined external symbols.
 * @param[in] level Message severity level
 * @param[in] message The error message to show.
 */
UNIJ_EXTERN void unij_show_message_impl(unij_level_t level, const wchar_t* message);

/**
 * @brief Application-specific termination of our logic.
 * In order to give the application/DLL full control over the display of messages and critical failure process, we leave
 * this and ::unij_show_message_impl as undefined external symbols.
 * NOTE: In the case of the loader DLLs, this shouldn't actually terminate the process. Instead, it should abort the
 * loader logic, then cleanup and unload the loader DLL.
 * @param[in] code Uniject-specific error code
 * @param[in] win32_error System error code (if any) 
 */
UNIJ_EXTERN void unij_abort_impl(unij_error_t code, uint32_t win32_error);

/**
 * @brief Gets a human readable description of the \a uCode. Use ::FreeErrorDescription for cleanup.
 * @param[in] code Error code 
 * @return Pointer to the description. Pass this into ::FreeErrorDescription when done with it.
 */
const wchar_t* unij_get_error_text(unij_error_t code);

/**
 * @brief Cleans up after a call to ::GetErrorDescription
 * @param[in] message Pointer previously returned by ::GetErrorDescription
 * @param[in] code Error code 
 */
void unij_free_error_text(unij_error_t code, const wchar_t* message);

/**
 * @brief Shows the user some kind of message. Not to be used for diagnostics. (intended for messages that the
 * user **should** see)
 * @param[in] level Message urgency level
 * @param[in] format Format string
 * @param[in] ... Var args for \a format 
 */
void unij_show_message(unij_level_t level, const wchar_t* format, ...);

/**
 * @def unij_show_error_message(format,...)
 * @brief Wrapper around ShowUserMessage that prefills ```UNIJ_LEVEL_ERROR``` for \a uLevel.
 * @param[in] format Format string
 * @param[in] ... Var args for \a format 
 */
#define unij_show_error_message(format,...) \
	unij_show_message(UNIJ_LEVEL_ERROR, UNIJ_TRACE_PREFIX(format), __VA_ARGS__ )

/**
 * @brief Fatal error message
 * @param[in] code Error code
 * @param[in] format Error format string
 * @param[in] ... Var args for \a format
 */
void unij_fatal_error(unij_error_t code, const wchar_t* format, ...);

/**
 * @brief Maps message level codes to text labels.
 * @param level Message level code
 * @return Level's text label. If code wasn't recognized, returns L"UNKNOWN"
 */
const unij_wstr_t* unij_level_name(unij_level_t level);

/**
 * @brief Wrapper around ::unij_abort_impl
 * @param[in] dwError System error code.
 * @return Expect this function to return.
 */
static UNIJ_INLINE void unij_abort(unij_error_t code)
{
	uint32_t win32_error = ERROR_SUCCESS;
	if(code == UNIJ_ERROR_LASTERROR)
		win32_error = GetLastError();
	unij_abort_impl(code, win32_error);
}

/**
 * @def unij_fatal_call(func)
 * @brief Convenience wrapper to ::unij_fatal_error for simplifying fatal errors caused by a failed API call.  
 * @param[in] func Name of the function that failed its call.
 */
#define unij_fatal_call(FUNC) \
	unij_fatal_error( \
		UNIJ_ERROR_LASTERROR, \
		L"%s: Failed call at %s:%s:%s", \
		UNIJ_WSTRINGIFY(FUNC), \
		UNIJ_WIDEN(__FILE__), \
		UNIJ_WSTRINGIFY(__LINE__), \
		__FUNCTIONW__ \
	)

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_ERROR_H_ */
