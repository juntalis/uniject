/**
 * @file mono_api.c
 */
#include <uniject.h>
#include <uniject/error.h>
#include <uniject/module.h>
#include <uniject/utility.h>
#include <uniject/win32.h>

#include "mono_api.h"

#define MONO_API(RET, NAME, ...) \
RET ( * NAME )( __VA_ARGS__ ) = NULL;
#include "mono_api.inl"

// One-time execution data for mono_api_init
static unij_once_t mono_api_initialized = UNIJ_ONCE_INIT;

// Used in \a mono_enable_debugging 
static const char mono_debug_argv[] = "--debugger-agent=transport=dt_socket,embedding=1,server=y,address=0.0.0.0:56000,defer=y"; 

static UNIJ_NOINLINE
BOOL CDECL mono_api_init_once(const unij_wstr_t* mono_path)
{
	HMODULE mono_module = unij_noref_module(mono_path->value);
	if(mono_module == NULL) {
		unij_fatal_error(UNIJ_ERROR_MONO, L"Failed to load mono DLL from: %s", mono_path->value);
		return FALSE;
	}
	
#	define MONO_API(RET, NAME, ...) \
	UNIJ_MESSAGE("Creating import code for mono." #NAME ": " #RET "(*)(" UNIJ_STRINGIFY(__VA_ARGS__)")" ); \
	*((FARPROC*)&NAME) = GetProcAddress(mono_module, #NAME ); \
	if(NAME == NULL) { \
		unij_fatal_error(UNIJ_ERROR_METHOD, L"Failed to locate required proc: mono.%s", UNIJ_WSTRINGIFY(NAME)); \
		return FALSE; \
	}
#	include "mono_api.inl"
	return TRUE;
}

bool mono_api_init(const unij_wstr_t* mono_path)
{
	return unij_once(&mono_api_initialized, (unij_once_fn)mono_api_init_once, (void*)mono_path);
}

static UNIJ_INLINE void mono_enable_logging(void)
{
	mono_unity_set_vprintf_func((vprintf_func)vprintf);
	mono_trace_set_level_string("debug");
	mono_trace_set_mask_string("all");
}

void mono_enable_debugging(void)
{
	char* jit_argv[1] = { (char*)mono_debug_argv };
	mono_enable_logging();
	mono_jit_parse_options(1, jit_argv);
	mono_debug_init(1);
}
