/**
 * @file winget_args.c
 * Implementation code for @file winget_args.h
 */
#include "pch.h"
#include "args.h"
#include "parg.h"

#define TOW(CH) \
	((wchar_t)(CH))

#define UNIJ_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define PARSE_ERROR_SUCCESS  0
#define PARSE_ERROR_EMPTY    1
#define PARSE_ERROR_INVALID  2
#define PARSE_ERROR_OVERFLOW 3

static struct parg_option const cli_options[] =
{
	{L"help",     PARG_NOARG,       NULL, L'h'},
	{L"list",     PARG_NOARG,       NULL, L'l'},
	{L"debug",    PARG_NOARG,       NULL, L'g'},
	{L"pid",      PARG_REQARG,      NULL, L'p'},
	{L"tid",      PARG_REQARG,      NULL, L't'},
	{L"class",    PARG_REQARG,      NULL, L'c'},
	{L"method",   PARG_REQARG,      NULL, L'm'},
	{L"mono",     PARG_REQARG,      NULL, L'M'},
	{NULL,       0,                 NULL, 0}
};

static bool test_hex_integer(const wchar_t** parg, size_t* plen)
{
	size_t idx = 0, len = *plen;
	wchar_t* arg = (wchar_t*)*parg;
	if(len == 0 || arg == NULL) {
		return false;
	} else if(len > 0 && arg[len - 1] == L'h') {
		*plen -= 1;
		arg[len - 1] = L'\0';
		return true;
	} else if(len > 2 && arg[1] == L'x') {
		*plen = len - 2;
		*parg = &(arg[2]);
		return true;
	}
	
	for(; idx < len; idx++)
		if(arg[idx] & TOW(0x40))
			return true;
	
	return false;
}

static UNIJ_INLINE bool is_digit(wchar_t ch)
{
	return (ch & TOW(0x10)) && (ch <= L'9');
}

static UNIJ_INLINE bool is_hexalpha(wchar_t ch)
{
	return (ch >= L'a') && (ch <= L'f');
}

static bool valid_dec_uint64(const wchar_t* arg, size_t len)
{
	size_t idx = 0;
	for(; idx < len; idx++) {
		if(!is_digit(arg[idx])) {
			wprintf(L"error: invalid character '%c' found in decimal integer argument '%s'\n", arg[idx], arg);
			return false;
		}
	}
	return true;
}

static bool valid_hex_uint64(const wchar_t* arg, size_t len)
{
	size_t idx = 0;
	for(; idx < len; idx++) {
		if(!is_digit(arg[idx]) && !is_hexalpha(arg[idx])) {
			wprintf(L"error: invalid character '%c' found in hexadecimal integer argument '%s'\n", arg[idx], arg);
			return false;
		}
	}
	return true;
}

static UNIJ_INLINE uint64_t parse_dec_uint64(int* errflag, const wchar_t* arg, size_t len)
{
	uint64_t result = 0;
	if(valid_dec_uint64(arg, len)) {
		result = (uint64_t)wcstoull(arg, NULL, 10);
	} else {
		*errflag = PARSE_ERROR_INVALID;
	}
	return result;
}

static UNIJ_INLINE wchar_t from_hex(wchar_t ch)
{
	return (ch & TOW(0x10)) ? TOW(ch - L'0') : TOW(ch - L'a' + 10);
}

static UNIJ_INLINE uint64_t parse_hex_uint64(int* errflag, const wchar_t* arg, size_t len)
{
	size_t idx = 0;
	uint64_t result = 0;
	if(valid_hex_uint64(arg, len)) {
		for(; idx < len; idx++) {
			size_t shift = (len - idx - 1) * 4;
			result |= ((uint64_t)from_hex(arg[idx])) << shift;
		}
	} else {
		*errflag = PARSE_ERROR_INVALID;
	}
	return result;
}

static uint64_t parse_uint64(int* errflag, const wchar_t* clarg)
{
	const wchar_t* arg;
	uint64_t result = 0;
	size_t arglen = clarg ? (size_t)lstrlenW(clarg) : 0;
	*errflag = PARSE_ERROR_SUCCESS;
	if(arglen == 0) {
		*errflag = PARSE_ERROR_EMPTY;
		return 0;
	}
	
	arg = (const wchar_t*)unij_strtolower((wchar_t*)clarg, arglen);
	if(test_hex_integer(&arg, &arglen)) {
		result = parse_hex_uint64(errflag, arg, arglen);
	} else {
		result = parse_dec_uint64(errflag, arg, arglen);
	}
	return result;
}

static uint32_t parse_uint32(int* errflag, const wchar_t* clarg)
{
	static const uint64_t max32 = (uint64_t)UINT32_MAX;
	uint64_t result64 = parse_uint64(errflag, clarg);
	if(*errflag != PARSE_ERROR_SUCCESS) return 0;
	if(result64 > max32) {
		*errflag = PARSE_ERROR_OVERFLOW;
		wprintf(L"error: integer '%llu' exceeds the maximum size for a 32-bit integer\n", result64);
		return 0;
	}
	return (uint32_t)(result64 & (uint64_t)0xFFFFFFFFUL);
}

static void parse_wstr(unij_wstr_t* dest, int* errflag, const wchar_t* arg)
{
	size_t arglen = arg ? (size_t)lstrlenW(arg) : 0;
	*errflag = PARSE_ERROR_SUCCESS;
	if(arglen == 0) {
		*errflag = PARSE_ERROR_EMPTY;
		return;
	} else if(arglen > UINT16_MAX) {
		*errflag = PARSE_ERROR_OVERFLOW;
		wprintf(L"error: string '%s' exceeds the maximum length\n", arg);
		return;
	}
	
	dest->value = arg;
	dest->length = (uint16_t)arglen;
}

static void parse_nonopts(unij_cliargs_t* argsobj, int argc, wchar_t** argv)
{
	if(argc == 1) {
		int errflag = PARSE_ERROR_SUCCESS;
		parse_wstr(&argsobj->params.assembly_path, &errflag, argv[0]);
		if(errflag != PARSE_ERROR_SUCCESS) usage(EXIT_FAILURE, argv[0], false);
	} else if(!argsobj->list) {
		wprintf(L"error: you must specify the path of the assembly you'd like to inject.\n");
		usage(EXIT_FAILURE, argv[0], false);
	}
}

void parse_args(unij_cliargs_t *argsobj, int argc, wchar_t* argv[])
{
	static const wchar_t optstring[] = L"hlgp:t:c:m:M:";
	int c, optend, errflag = PARSE_ERROR_SUCCESS, optind = 0;
	struct parg_state ps = {NULL};
	
	// Check for empty command line
	if(argc == 1) {
		usage(EXIT_SUCCESS, argv[0], true);
	}
	
	parg_init(&ps);
	RtlZeroMemory((void*)argsobj, sizeof(*argsobj));
	//optend = parg_reorder(argc, argv, optstring, NULL);
	while((c = parg_getopt_long(&ps, argc, argv, optstring, cli_options, &optind)) != -1) {
		switch(c) {
			case 1:
				parse_wstr(&argsobj->params.assembly_path, &errflag, ps.optarg);
				break;
			case L'h':
				argsobj->help = true;
				usage(EXIT_SUCCESS, argv[0], true);
				break;
			case L'l':
				argsobj->list = true;
				break;
			case L'g':
				argsobj->params.debugging = true;
				break;
			case L'p':
				argsobj->params.pid = parse_uint32(&errflag, ps.optarg);
				break;
			case L't':
				argsobj->params.tid = parse_uint32(&errflag, ps.optarg);
				break;
			case L'c':
				parse_wstr(&argsobj->params.class_name, &errflag, ps.optarg);
				break;
			case L'm':
				parse_wstr(&argsobj->params.method_name, &errflag, ps.optarg);
				break;
			case L'M':
				parse_wstr(&argsobj->params.mono_path, &errflag, ps.optarg);
				break;
			default:
				static const wchar_t null_text[] = L"(null)";
				wprintf(L"error: unhandled option -%c\n", (wchar_t)c);
				wprintf(L"optind = %d\n", optind);
				wprintf(L"ps =>\n");
				wprintf(L"  optarg   = \"%s\"\n", ps.optarg ? ps.optarg : null_text);
				wprintf(L"  nextchar = \"%s\"\n", ps.nextchar ? ps.nextchar : null_text);
				wprintf(L"  optind   = %d\n", ps.optind);
				wprintf(L"  optopt   = %d\n", ps.optopt);
				usage(EXIT_FAILURE, argv[0], false);
				break;
		}
		
		if(errflag == PARSE_ERROR_EMPTY) {
			int errind = UNIJ_MAX(ps.optind, optind);
			wprintf(L"error: parametrer %s requires a non-emptyy value!\n", cli_options[errind].name);
		}
		if(errflag != PARSE_ERROR_SUCCESS) {
			break;
		}
	}
	
	if(errflag) {
		usage(EXIT_FAILURE, argv[0], false);
	}
	
	optend = UNIJ_MAX(optind, ps.optind);
	if(optend < argc) {
		parse_nonopts(argsobj, argc - optend, &argv[optind]);
	}
	
	if(!argsobj->list) {
		if(!argsobj->params.pid) {
			wprintf(L"error: you must specify the PID of the process you're looking to target.\n");
			usage(EXIT_FAILURE, argv[0], false);
		}
		if(unij_is_empty(&argsobj->params.assembly_path)) {
			wprintf(L"error: you must specify the path of the assembly you'd like to inject.\n");
			usage(EXIT_FAILURE, argv[0], false);
		}
	}
}

UNIJ_NORETURN usage(int status, const wchar_t* program_name, bool show)
{
	if(!show) {
		wprintf(L"Try '%s --help' for more information.\n", program_name);
	} else {
		wprintf(
L"Usage: %s -l\n"
L"  or:  %s [OPTION]... -p PID ASSEMBLY\n"
L"\n"
L"  -h, --help                     display this help and exit\n"
L"  -V, --version                  output version information and exit\n"
L"  -l, --list                     list active unity processes\n"
L"\n"
L"Injection parameters:\n"
L"   ASSEMBLY                      filepath of the injected assembly\n"
L"  -p, --pid                      required process id\n"
L"  -g, --debug                    enable release-mode debugging\n"
L"  -t, --tid                      optional thread id\n"
L"  -c, --class                    targeted class name (default: Loader)\n"
L"  -m, --method                   targeted method name (default: Initialize)\n"
L"  -M, --mono                     mono dll filepath (default: autodetected)\n",
		program_name, program_name);
	}
	exit(status);
}


