/**
 * @file mono_api.c
 */
#include <uniject.h>
#include "mono_api.h"
#include <uniject/win32.h>
#include <uniject/utility.h>

#define MONO_API(RET, NAME, ...) \
imp_ ## NAME NAME = NULL;
#include "mono_api.inl"

static void mono_api_error_default(uint32_t code, const char* message);

static mono_api_error_fn error_handler = mono_api_error_default;

static void mono_api_error_default(uint32_t code, const char* message)
{
	const char* formatted = (const char*)saprintf(NULL, "ERROR[%u]: %s\n", code, message);
	OutputDebugStringA(formatted);
	free((void*)formatted);
}

bool mono_api_init(const wchar_t* filepath)
{
	bool result = true;
	HMODULE hModule = unij_get_module(filepath);
	if(hModule == NULL) {
		return false;
	}
	
#	define MONO_API(RET, NAME, ...) \
	CC_MESSAGE("Creating import code for mono." #NAME ": " #RET "()(" CC_STR(CC_SINGLE_ARG(__VA_ARGS__))")" ); \
	NAME = ( imp_ ## NAME )GetProcAddress(hModule, #NAME ); \
	if(NAME == NULL) { \
		result = false; \
		mono_api_report_error((uint32_t)GetLastError(), "GetProcAddress(" #NAME ")"); \
	}
#	include "mono_api.inl"
	
	return result;

}

void mono_api_set_error_handler(mono_api_error_fn handler)
{
	error_handler = handler == NULL ? mono_api_error_default : handler;
}

void mono_api_report_error(uint32_t code, const char* message)
{
	error_handler(code, message);
}
