// 64-bit injection code test
#ifndef _UNICODE
#	define _UNICODE 1
#	define UNICODE 1
#endif

#ifdef _MBCS
#	undef _MBCS
#endif

#define _CRT_RAND_S  
#include <windows.h>
#include <winternl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#define UNIJ_INLINE 
// #define UNIJ_INLINE __forceinline
#define UNIJ_TLS __declspec(thread)
#define UNIJ_DLLIMP __declspec(dllimport)

/* Token pasting macro */
#define _UNIJ_PASTE2(A,B) A##B
#define UNIJ_PASTE(A,B) _UNIJ_PASTE2(A,B)

#if defined(__x86_64__) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_X64)
#	define ARCH_X64
#	define ARCH_BITS 64
#	define ARCH_IP Rip
#else
#	define ARCH_X86
#	define ARCH_BITS 32
#	define ARCH_IP Eip
#endif

#ifdef ARCH_X86

static const uint8_t HIJACK_TEMPLATE[] = {
	                              // get_eip:
	0x8B, 0x04, 0x24,             //   mov eax, [esp]
	0xC3,                         //   retn
	                              // entrypoint:
	0x68, 0, 0, 0, 0,             //   push original_eip
	0x9C,                         //   pushf
	0x60,                         //   pusha
	0xE8, 0xF0, 0xFF, 0xFF, 0xFF, //   call get_eip
	0x8D, 0x40, 0x14,             //   lea eax, [eax + callback_fn - $]
	0x8B, 0x38,                   //   mov edi, [eax]
	0x8B, 0x40, 0x04,             //   mov eax, [eax+4]
	0x50,                         //   push eax
	0xFF, 0xD7,                   //   call edi
	0x83, 0xC4, 0x04,             //   add esp, 4
	0x61,                         //   popa
	0x9D,                         //   popf
	0xC3,                         //   retn
	0, 0, 0                       // xx - padding
};

#define HIJACK_TEMPLATE_OFFSET_0 0x05
#define HIJACK_TEMPLATE_ENTRYPOINT 0x04

#else
	
static const uint8_t HIJACK_TEMPLATE[] = {
	                                          // entrypoint:
	0x9C,                                     //   pushfq
	0x50,                                     //   push rax
	0x51,                                     //   push rcx
	0x52,                                     //   push rdx
	0x53,                                     //   push rbx
	0x55,                                     //   push rbp
	0x56,                                     //   push rsi
	0x57,                                     //   push rdi
	0x41, 0x50,                               //   push r8
	0x41, 0x51,                               //   push r9
	0x41, 0x52,                               //   push r10
	0x41, 0x53,                               //   push r11
	0x41, 0x54,                               //   push r12
	0x41, 0x55,                               //   push r13
	0x41, 0x56,                               //   push r14
	0x41, 0x57,                               //   push r15
	0x48, 0x83, 0xEC, 0x28,                   //   sub rsp, 28h
	0x48, 0x8D, 0x0D, 0x3D, 0x00, 0x00, 0x00, //   lea rcx, callback_data
	0xFF, 0x15, 0x2F, 0x00, 0x00, 0x00,       //   call cs:callback_fn
	0x48, 0x83, 0xC4, 0x28,                   //   add rsp, 28h
	0x41, 0x5F,                               //   pop r15
	0x41, 0x5E,                               //   pop r14
	0x41, 0x5D,                               //   pop r13
	0x41, 0x5C,                               //   pop r12
	0x41, 0x5B,                               //   pop r11
	0x41, 0x5A,                               //   pop r10
	0x41, 0x59,                               //   pop r9
	0x41, 0x58,                               //   pop r8
	0x5F,                                     //   pop rdi
	0x5E,                                     //   pop rsi
	0x5D,                                     //   pop rbp
	0x5B,                                     //   pop rbx
	0x5A,                                     //   pop rdx
	0x59,                                     //   pop rcx
	0x58,                                     //   pop rax
	0x9D,                                     //   popfq
	0xFF, 0x25, 0x05, 0x00, 0x00, 0x00,       //   jmp cs:original_rip
	0, 0, 0, 0, 0,                            // xx - padding
	                                          //
	0, 0, 0, 0, 0, 0, 0, 0,                   // original_rip: dq 0
};

#define HIJACK_TEMPLATE_OFFSET_0 0x50
#define HIJACK_TEMPLATE_ENTRYPOINT 0x00

#endif

#define HIJACK_TEMPLATE_SIZE sizeof(HIJACK_TEMPLATE)

// CRT Stuff

#undef RtlFillMemory
#undef RtlZeroMemory
#undef RtlMoveMemory
#undef RtlCopyMemory
#undef RtlEqualMemory
#undef RtlCompareMemory

typedef ULONG LOGICAL;

struct unij_wstr
{
	uint16_t length;
	const wchar_t* value;
};

struct hijack_params
{
	void(CDECL* callback_fn)(LPCWSTR);
	LPCWSTR callback_data;
};

typedef struct unij_wstr unij_wstr_t;
typedef struct hijack_params hijack_params_t;

UNIJ_DLLIMP void WINAPI RtlFillMemory(PVOID, SIZE_T, BYTE);
UNIJ_DLLIMP void WINAPI RtlZeroMemory(PVOID, SIZE_T);
UNIJ_DLLIMP void WINAPI RtlMoveMemory(PVOID, const VOID*, SIZE_T);
UNIJ_DLLIMP LOGICAL WINAPI RtlEqualMemory(const VOID*, const VOID*, SIZE_T);
UNIJ_DLLIMP SIZE_T WINAPI RtlCompareMemory(const VOID*, const VOID*, SIZE_T);

#ifdef ARCH_X64
UNIJ_DLLIMP void WINAPI RtlCopyMemory(PVOID, const VOID*, SIZE_T);
#else
#	define RtlCopyMemory RtlMoveMemory
#endif

#define WSIZE(COUNT) \
	((size_t)( (COUNT) * sizeof(wchar_t) ))

#define WLEN(WSTR) \
	((sizeof(WSTR) / sizeof(*(WSTR))) - 1)

#define DEFINE_STATIC_WSTR(VAR,TEXT) \
	static const wchar_t UNIJ_PASTE(VAR,_value) [] = TEXT ; \
	static unij_wstr_t VAR = { \
		WLEN(TEXT), \
		UNIJ_PASTE(VAR,_value) \
	}

#define MAKE_PTR(BUF,OFFSET) \
	((PVOID)(((ULONG_PTR)(PVOID)(BUF)) + ((ULONG_PTR)(OFFSET)) ))

#define DUMMY_WAITMS 1000
#define HIJACK_DELAY (DUMMY_WAITMS * 3)

static BOOL bLoopStop = FALSE;
static const wchar_t pCallbackData[] = L"INSIDE dummy_thread!!!";

#define LOG_PREFIX_LENGTH \
	WLEN(L"[TID:00000000] ")

UNIJ_TLS uint32_t tlsTID = 0;
UNIJ_TLS uint32_t tlsRndSeed = 0;
UNIJ_TLS unij_wstr_t tlsLogPrefix = { 0, NULL };
UNIJ_TLS const wchar_t tlsLogPrefixText[LOG_PREFIX_LENGTH+1] = {0};

#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 0

static BOOL ensure_tls(void)
{
	uint64_t abc = 0;
	PULARGE_INTEGER xyz = (PULARGE_INTEGER)&abc;
	if(tlsTID == 0) {
		DWORD dwTID = GetCurrentThreadId();
		if(dwTID > 0) {
			tlsTID = dwTID;
			tlsLogPrefix.value = tlsLogPrefixText;
			tlsLogPrefix.length = LOG_PREFIX_LENGTH;
			swprintf((wchar_t*)tlsLogPrefixText, LOG_PREFIX_LENGTH+1, L"[TID:%08lX] ", dwTID);
		}
	}
	return tlsTID != 0;
}

static UNIJ_INLINE DWORD get_thread_id(void)
{
	return ensure_tls() ? tlsTID : 0;
}

static UNIJ_INLINE unij_wstr_t get_thread_prefix(void)
{
	return ensure_tls() ? tlsLogPrefix : (unij_wstr_t){ 0, NULL };
}

static UNIJ_INLINE unij_wstr_t get_level_prefix(int level)
{
	DEFINE_STATIC_WSTR(error_prefix, L"ERROR: ");
	DEFINE_STATIC_WSTR(info_prefix, L"INFO: ");
	DEFINE_STATIC_WSTR(invalid_prefix, L"INVALID-LEVEL: ");

	switch(level)
	{
		case LOG_LEVEL_ERROR:
			return error_prefix;
		case LOG_LEVEL_INFO:
			return info_prefix;
		default:
			return invalid_prefix;
	}
}

#define COPY_WCHARS(DEST,SRC,LEN) \
	copy_wchars( \
		(void*)(DEST), \
		(const wchar_t*)(SRC), \
		(LEN) \
	)

static UNIJ_INLINE void* copy_wchars(void* dest, const wchar_t* src, size_t count)
{
	size_t szCopy = WSIZE(count);
	RtlCopyMemory(dest, (const void*)src, szCopy);
	return (void*)((size_t)(dest) + szCopy);
}

static UNIJ_INLINE void* copy_wstr(void* dest, unij_wstr_t* src)
{
	return COPY_WCHARS(dest, src->value, src->length);
}

#define LOG_INFO(...) \
	(log_line(TRUE, LOG_LEVEL_INFO, __VA_ARGS__ ))

#define LOG_ERROR(...) \
	(log_line(TRUE, LOG_LEVEL_ERROR, __VA_ARGS__ ))

static CRITICAL_SECTION csLogLine;

static void log_line(BOOL uselock, int level, const wchar_t* format, ...)
{
	va_list args;
	void* pformat;
	BOOL newline = FALSE;
	const wchar_t *format_buffer;
	size_t format_len, buffer_len, buffer_size;
	unij_wstr_t thread_prefix, level_prefix;
	level_prefix = get_level_prefix(level);
	thread_prefix = get_thread_prefix();
	if(!thread_prefix.length || ! level_prefix.length) {
		wprintf(L"WARNING: log_line called without valid format or level.");
		return;
	}
	
	format_len = (size_t)lstrlenW(format);
	buffer_len = format_len + (size_t)(thread_prefix.length + level_prefix.length + 1);
	newline = format[format_len -1] != L'\n';
	if(newline) buffer_len += 1;
	buffer_size = WSIZE(buffer_len);
	
	format_buffer = (const wchar_t*)malloc(buffer_size);
	if(format_buffer == NULL) {
		perror("malloc");
		exit(1);
	} else {
		RtlZeroMemory((PVOID)format_buffer, buffer_size);
	}

	assert(format_buffer != NULL);
	
	// Copy over each part into our new format string buffer
	pformat = (void*)format_buffer;
	pformat = copy_wstr(pformat, &thread_prefix);
	pformat = copy_wstr(pformat, &level_prefix);
	pformat = COPY_WCHARS(pformat, format, format_len);
	if(newline)  *((wchar_t*)pformat) = L'\n';
	
	// Execute IO within a critical section
	
	if(uselock)
		EnterCriticalSection(&csLogLine);
	
	// Now write to stdout
	va_start(args, format);
	vwprintf(format_buffer, args);
	va_end(args);
	fflush(stdout);
	
	// All done
	if(uselock)
		LeaveCriticalSection(&csLogLine);
	
	// Cleanup
	free((void*)format_buffer);
}

#define log_info LOG_INFO
#define log_failure LOG_ERROR


static uint32_t rand_range(uint32_t rmin, uint32_t rmax)
{
	
}

static void CDECL hijack_callback(LPCWSTR data)
{
	// Potential for a deadlock if I use the critical section here
	log_line(FALSE, LOG_LEVEL_INFO, L"hijack_callback called!");
	log_line(FALSE, LOG_LEVEL_INFO, L"Received callback data: \"%s\"\n", data);
	bLoopStop = TRUE;
}

static DWORD WINAPI dummy_thread_proc(LPVOID lpParameter)
{
	DWORD dwLoopCount = 0;
	while(!bLoopStop)
	{
		Sleep(DUMMY_WAITMS);
		log_info(L"Iteration %lu..\n", dwLoopCount++);
	}
	return 1;
}

PVOID alloc_hijack_buffer(PVOID source, SIZE_T size)
{
	PVOID result = NULL;
	HANDLE hProcess = GetCurrentProcess();
	log_info(L"Allocating hijack code buffer..");
	result = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!WriteProcessMemory(hProcess, result, source, size, NULL)) {
		DWORD dwLastError = GetLastError();
		VirtualFreeEx(hProcess, result, size, MEM_RELEASE);
		result = NULL;
		SetLastError(dwLastError);
		log_failure(L"WriteProcessMemory!");
	} else {
		FlushInstructionCache(hProcess, result, size);
	}
	return result;
}

BOOL inject_thread(HANDLE hThread, hijack_params_t* params)
{
	PVOID pCode = NULL;
	CONTEXT context = {0};
	BOOL bResult = FALSE;
	DWORD dwExitCode, dwThreadId = 0;
	SIZE_T szBuffer = HIJACK_TEMPLATE_SIZE + sizeof(hijack_params_t);
	BYTE pBuffer[HIJACK_TEMPLATE_SIZE + sizeof(hijack_params_t)] = {0};
	
	PVOID pHijack = MAKE_PTR(pBuffer, HIJACK_TEMPLATE_SIZE);
	ULONG_PTR* pIP =  (ULONG_PTR*)MAKE_PTR(pBuffer, HIJACK_TEMPLATE_OFFSET_0);
	
	// Wait 3x dummy's iter delay
	log_info(L"Suspending target thread in %dms..", HIJACK_DELAY);
	Sleep(HIJACK_DELAY);
	log_info(L"Suspending..");
	SuspendThread(hThread);
	
	context.ContextFlags = CONTEXT_CONTROL;
	GetThreadContext(hThread, &context);
	
	log_info(L"Rendering shellcode locally..");
	// Copy over the template bytes, then overlay the substituted stuff
	RtlCopyMemory((void*)pBuffer, (const void*)HIJACK_TEMPLATE, HIJACK_TEMPLATE_SIZE);
	*pIP = (ULONG_PTR)context.ARCH_IP;
	RtlCopyMemory(pHijack, (const void*)params, sizeof(*params));
	
	
	log_info(L"Allocating virtual memory and populating..");
	// Allocate buffer, map real entrypoint
	pCode = alloc_hijack_buffer(pBuffer, szBuffer);
	
	// Alter eip & resume
	
	log_info(L"Resuming thread..");
	context.ARCH_IP = (ULONG_PTR)MAKE_PTR(pCode, HIJACK_TEMPLATE_ENTRYPOINT);
	SetThreadContext(hThread, &context);
	ResumeThread(hThread);
	
	log_info(L"Waiting for thread completion..");
	WaitForSingleObject(hThread, INFINITE);
	if(!GetExitCodeThread(hThread, &dwExitCode)) {
		log_failure(L"GetExitCodeThread");
		goto cleanup;
	}
	
	log_info(L"Done.");
	bResult = dwExitCode != 0;
	
cleanup:
	
	if(pCode != NULL) {
		VirtualFreeEx(GetCurrentProcess(), pCode, szBuffer, MEM_RELEASE);
		pCode = NULL;
	}
	
	if(bResult) {
		SetLastError(ERROR_SUCCESS);
	}
	return bResult;
}

static HANDLE create_dummy_thread(void)
{
	HANDLE hThread;
	DWORD dwThreadId;
	log_info(L"Creating dummy thread to hijack..");
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)dummy_thread_proc, NULL, 0, &dwThreadId);
	if(hThread == NULL || hThread == INVALID_HANDLE_VALUE) {
		log_failure(L"CreateThread");
		return NULL;
	}
	return hThread;
}

int wmain(int argc, wchar_t *argv[])
{
	BOOL bResult;
	DWORD dwError;
	HANDLE hThread;
	hijack_params_t params = { hijack_callback, pCallbackData };
	
	InitializeCriticalSection(&csLogLine);
	
	hThread = create_dummy_thread();
	if(hThread == NULL) {
		return 1;
	}
	
	bResult = inject_thread(hThread, &params);
	dwError = GetLastError();
	
	if(bResult) {
		log_info(L"We won!");
	} else {
		log_info(L"We lost!");
	}
	
	CloseHandle(hThread);
	if(dwError != ERROR_SUCCESS) {
		log_failure(L"Error detected: %lu\n", dwError);
		return 1;
	}
	
	DeleteCriticalSection(&csLogLine);
	
	return bResult ? 0 : 1;
}

#ifdef _WINDOWS

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpOriginalCmdLine, int nCmdShow)
{
	if(!AllocConsole()) {
		
	}
	return wmain(__argc, __wargv);
}

#endif
