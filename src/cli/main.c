/**
 * @file main.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "args.h"
#include <uniject/injector.h>
#include <uniject/process.h>

// TODO: Atomics compatibility macros
#pragma intrinsic(_InterlockedCompareExchange)

typedef struct unij_errors unij_errors_t;

struct unij_errors
{
	unij_error_t unij_code;
	uint32_t win32_code;
};

// Persistent uniject-specific error information for the first fatal error encountered.
static UNIJ_CACHE_ALIGN uint64_t cli_exit_code = 0;

static void unij_set_exit_codes(unij_errors_t codes)
{
	// just in case..
	uint64_t error = MAKEULONGLONG(codes.unij_code, codes.win32_code);
	if(_InterlockedCompareExchange64((volatile int64_t*)&cli_exit_code, (int64_t)error, 0) == 0) {
		SetLastError(ERROR_SUCCESS);
	}
}

static unij_errors_t unij_get_fatal_codes(void)
{
	uint64_t error = *((volatile uint64_t*)&cli_exit_code);
	uint32_t* codes = (uint32_t*)&error; // CLion bitching when I try a simple cast
	return (unij_errors_t) {
		(unij_error_t)codes[0], codes[1]
	};
}

static uint32_t unij_get_exit_code(void)
{
	uint64_t error = *((volatile uint64_t*)&cli_exit_code);
	uint32_t* codes = (uint32_t*)&error; // CLion bitching when I try a simple cast
	return codes[1];
}

static int cmd_inject(unij_cliargs_t* args)
{
	int result = EXIT_SUCCESS;
	const unij_params_t* params = &args->params;
	uniject_t* ctx = unij_injector_open_params(params);
	if(ctx == NULL || !unij_inject(ctx)) {
		result = unij_get_exit_code() || EXIT_FAILURE;
		wprintf(L"Injection failed: %d\n", result);
	} else {
		wprintf(L"Injection completed successfully.\n");
	}
	unij_close(ctx);
	return result;
}

static UNIJ_NOINLINE bool CDECL cmd_list_monoinfo_fn(unij_monoinfo_t* info, void* parameter)
{
	UNIJ_SUPPRESS_UNUSED(parameter);
	wprintf(L"%u\t%s\t%s\n", info->pid, info->exe_path, info->mono_name);
	return false;
}

static int  cmd_list(void)
{
	wprintf(L"Currently running Unity processes:\n");
	unij_enum_mono_processes(cmd_list_monoinfo_fn, NULL);
	return 0;
}

int wmain(int argc, wchar_t* argv[])
{
	unij_cliargs_t cliargs = {false};
	parse_args(&cliargs, argc, argv);
	if(cliargs.list) {
		return cmd_list();
	} else {
		return cmd_inject(&cliargs);
	}
}
 
// sizeof("[WARNING]")
#define LEVEL_PREFIX_MAX 10
#define LEVEL_PREFIX_FORMAT L"%-" UNIJ_WSTRINGIFY(LEVEL_PREFIX_MAX) L"s" 

// Implementation for uniject library's usage
void unij_show_message_impl(unij_level_t level, const wchar_t* message)
{
	wchar_t level_prefix[LEVEL_PREFIX_MAX] = L"[";
	const unij_wstr_t* level_name = unij_level_name(level);
	lstrcatW(level_prefix, level_name->value);
	lstrcatW(level_prefix, L"]");
	wprintf(LEVEL_PREFIX_FORMAT L" %s\n", level_prefix, message);
}

void unij_abort_impl(unij_error_t code, uint32_t win32_error)
{
	unij_errors_t errors = { code, win32_error };
	unij_set_exit_codes(errors);
	// ???
}
