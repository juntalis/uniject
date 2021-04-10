/**
 * @file error.c
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Contains the necessary API implementations from \a uniject/error.h
 */
#include "pch.h"
#include "error.h"

#include <uniject/module.h>

// TODO: Atomics compatibility macros
#pragma intrinsic(_InterlockedCompareExchange64)

// One-time execution data for ensure_msgbox
static unij_once_t msgbox_ensured = UNIJ_ONCE_INIT;

// Persistent uniject-specific error information for the first fatal error encountered.
static UNIJ_CACHE_ALIGN uint64_t first_fatal_error = 0; 

// User32 HMODULE and GetMessageBoxW pointers
struct msgbox
{
	HMODULE module;
	int(WINAPI* fn)(HWND, LPCWSTR, LPCWSTR, UINT);
};

static struct msgbox loaded_msgbox = { NULL, NULL };

static UNIJ_NOINLINE
BOOL CDECL ensure_msgbox_once(struct msgbox* loaded)
{
	loaded->module = LoadLibraryW(L"user32.dll");
	*((FARPROC*)&loaded->fn) = GetProcAddress(loaded->module, "MessageBoxW");
	return loaded->fn != NULL;
}

static UNIJ_INLINE bool ensure_msgbox(void)
{
	return unij_once(&msgbox_ensured, (unij_once_fn)ensure_msgbox_once, (void*)&loaded_msgbox) && loaded_msgbox.fn;
}

void unij_error_init(void)
{
	ensure_msgbox();
}

void unij_error_shutdown(void)
{
	loaded_msgbox.fn = NULL;
	if(loaded_msgbox.module != NULL) {
		FreeLibrary(loaded_msgbox.module);
		loaded_msgbox.module = NULL;
	}
}

void unij_set_fatal_error(unij_errors_t codes)
{
	// just in case..
	uint64_t error = MAKEULONGLONG(codes.unij_code, codes.win32_code);
	if(_InterlockedCompareExchange64((volatile int64_t*)&first_fatal_error, (int64_t)error, 0) == 0) {
		SetLastError(ERROR_SUCCESS);
	}
}

unij_errors_t unij_get_fatal_error(void)
{
	uint64_t error = *((volatile uint64_t*)&first_fatal_error);
	uint32_t* codes = (uint32_t*)&error; // CLion bitching when I try a simple cast
	return (unij_errors_t) {
		(unij_error_t)codes[0], codes[1]
	};
}

static UNIJ_INLINE void show_message_msgbox_impl(unij_level_t level, const wchar_t* message)
{
	UINT flags = MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST;
	switch(level)
	{
		case UNIJ_LEVEL_FATAL:
			flags |= MB_ICONERROR;
			break;
#   ifdef _NDEBUG
		default:
			return;
#   else
		case UNIJ_LEVEL_INFO:
			flags |= MB_ICONINFORMATION;
			break;
		case UNIJ_LEVEL_ERROR:
		case UNIJ_LEVEL_WARNING:
			flags |= MB_ICONEXCLAMATION;
			break;
		default:
			flags |= MB_ICONQUESTION;
			break;
#   endif
	}
	
	loaded_msgbox.fn(NULL, message,  L"Uniject", flags);
}

static UNIJ_INLINE void show_message_fallback_impl(unij_level_t level, const wchar_t* message)
{
	wchar_t* debug_message;
	size_t debug_message_len;
	const unij_wstr_t* name = unij_level_name(level);
	debug_message_len = (size_t)name->length + (size_t)lstrlenW(message) + sizeof(": ");
	debug_message = unij_wcsalloc(debug_message_len);
	assert(debug_message != NULL);
	RtlCopyMemory(debug_message, name->value, WSIZE(name->length));
	lstrcatW(debug_message, L": ");
	lstrcatW(debug_message, message);
	OutputDebugStringW((const wchar_t*)debug_message);
	unij_free((void*)debug_message);
}

// Implementation for uniject library's usage
void unij_show_message_impl(unij_level_t level, const wchar_t* message)
{
	if(ensure_msgbox()) {
		show_message_msgbox_impl(level, message);
	} else {
		show_message_fallback_impl(level, message);
	}
}

static UNIJ_NOINLINE HMODULE this_module(void)
{
	return unij_noref_module_ex((const wchar_t*)&this_module, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS);
}

void unij_abort_impl(unij_error_t code, uint32_t win32_error)
{
	unij_errors_t errors = { code, win32_error };
	unij_set_fatal_error(errors);
	// ???
	unij_show_message(UNIJ_LEVEL_ERROR, L"unij_abort called with codes %u:%u", code, win32_error);
	if(win32_error != 0) {
		FreeLibraryAndExitThread(this_module(), (DWORD)win32_error);
	}
}

