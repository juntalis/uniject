// 64-bit injection code test

#include <windows.h>
#include <winternl.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__x86_64__) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_X64)
#	define ARCH_X64
#	define ARCH_BITS 64
#else
#	define ARCH_X86
#	define ARCH_BITS 32
#endif

#define UNIJ_DLLIMP __declspec(dllimport)

#undef RtlFillMemory
#undef RtlZeroMemory
#undef RtlMoveMemory
#undef RtlCopyMemory
#undef RtlEqualMemory
#undef RtlCompareMemory

typedef ULONG LOGICAL;

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

static const uint8_t INJECT_TEMPLATE32[] = {
	                              // entrypoint:
	0x55,                         //   push ebp
	0x89, 0xE5,                   //   mov ebp, esp
	0x56,                         //   push esi
	0x57,                         //   push edi
	0x8B, 0x45, 0x08,             //   mov eax, [ebp+arg_0]
	0x50,                         //   push eax
	0x31, 0xC9,                   //   xor ecx, ecx
	0x64, 0x8B, 0x71, 0x30,       //   mov esi, fs:[ecx+30h]
	0x8B, 0x76, 0x0C,             //   mov esi, [esi+0Ch]
	0x8B, 0x76, 0x1C,             //   mov esi, [esi+1Ch]
	0x8B, 0x6E, 0x08,             //   mov ebp, [esi+8]
	0x8B, 0x7E, 0x20,             //   mov edi, [esi+20h]
	0x8B, 0x36,                   //   mov esi, [esi]
	0x66, 0x39, 0x4F, 0x18,       //   cmp [edi+12*2], cx
	0x75, 0xF2,                   //   jnz short next_module
	0x89, 0xEF,                   //   mov edi, ebp
	0xBE, 0x00, 0x00, 0x00, 0x00, //   mov esi, loadlib_rva
	0x01, 0xF7,                   //   add edi, esi
	0xFF, 0xD7,                   //   call edi
	0x5F,                         //   pop edi
	0x5E,                         //   pop esi
	0x5D,                         //   pop ebp
	0xC2, 0x04, 0x00,             //   retn 4
};

#define INJECT_TEMPLATE32_OFFSET_0 0x26
#define INJECT_TEMPLATE32_SIZE sizeof(INJECT_TEMPLATE32)

static const uint8_t INJECT_TEMPLATE64[] = {
	                              // entrypoint:
	0x53,                         //   push rbx
	0x56,                         //   push rsi
	0x57,                         //   push rdi
	0x55,                         //   push rbp
	0x48, 0x83, 0xEC, 0x28,       //   sub rsp, 28h
	0x48, 0x31, 0xD2,             //   xor rdx, rdx
	0x65, 0x48, 0x8B, 0x72, 0x60, //   mov rsi, gs:[rdx+60h]
	0x48, 0x8B, 0x76, 0x18,       //   mov rsi, [rsi+18h]
	0x48, 0x8B, 0x76, 0x10,       //   mov rsi, [rsi+10h]
	0x48, 0xAD,                   //   lodsq
	0x48, 0x8B, 0x30,             //   mov rsi, [rax]
	0x48, 0x8B, 0x7E, 0x30,       //   mov rdi, [rsi+30h]
	0xBE, 0x00, 0x00, 0x00, 0x00, //   mov esi, loadlib_rva
	0x48, 0x01, 0xF7,             //   add rdi, rsi
	0xFF, 0xD7,                   //   call rdi
	0x48, 0x83, 0xC4, 0x28,       //   add rsp, 28h
	0x5D,                         //   pop rbp
	0x5F,                         //   pop rdi
	0x5E,                         //   pop rsi
	0x5B,                         //   pop rbx
	0xC3,                         //   retn
};

#define INJECT_TEMPLATE64_OFFSET_0 0x22
#define INJECT_TEMPLATE64_SIZE sizeof(INJECT_TEMPLATE64)

static const wchar_t TESTDLL_NAME32[] = L"test32.dll";
static const wchar_t TESTDLL_NAME64[] = L"test64.dll";

#define TESTDLL_NAME32_SIZE sizeof(TESTDLL_NAME32)
#define TESTDLL_NAME64_SIZE sizeof(TESTDLL_NAME64)

#ifdef ARCH_X64
#	define TESTDLL_NAME TESTDLL_NAME64
#	define TESTDLL_NAME_SIZE TESTDLL_NAME64_SIZE

#	define INJECT_TEMPLATE INJECT_TEMPLATE64
#	define INJECT_TEMPLATE_SIZE INJECT_TEMPLATE64_SIZE
#	define INJECT_TEMPLATE_OFFSET_0 INJECT_TEMPLATE64_OFFSET_0
#else
#	define TESTDLL_NAME TESTDLL_NAME32
#	define TESTDLL_NAME_SIZE TESTDLL_NAME32_SIZE

#	define INJECT_TEMPLATE INJECT_TEMPLATE32
#	define INJECT_TEMPLATE_SIZE INJECT_TEMPLATE32_SIZE
#	define INJECT_TEMPLATE_OFFSET_0 INJECT_TEMPLATE32_OFFSET_0
#endif

#define _PTR(X) ((uintptr_t)(X))

static void failure(const wchar_t* message)
{
	wprintf(L"ERROR: %s\n", message);
}

static uint32_t get_loadlib_rva(void)
{
	HMODULE hKernel32 = NULL;
	uintptr_t loadlib_proc = 0;
	hKernel32 = GetModuleHandleW(L"kernel32.dll");
	loadlib_proc = _PTR(GetProcAddress(hKernel32, "LoadLibraryW"));
	return (uint32_t)(
		loadlib_proc - _PTR(hKernel32)
	);
}

static BOOL inject_thread(HANDLE hProcess, const void* buffer, size_t size)
{
	void* pdll = NULL;
	void* pmem = NULL;
	BOOL bResult = FALSE;
	HANDLE hThread = NULL;
	DWORD dwThreadId = 0, dwExitCode = 0;
	
	pmem = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!WriteProcessMemory(hProcess, pmem, buffer, size, NULL)) {
		failure(L"Failed call to WriteProcessMemory!");
		goto cleanup;
	}
	
	// Grab the remote pointer to the dll path
	pdll = (void*)(_PTR(pmem) + INJECT_TEMPLATE_SIZE);
	
	// Flush instructions and start the thread
	FlushInstructionCache(hProcess, pmem, size);
	wprintf(L"INFO: Creating thread..\n");
	hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)pmem,
		pdll,
		0,
		&dwThreadId
	);
	
	// Check created thread.
	if(hThread == NULL || hThread == INVALID_HANDLE_VALUE) {
		failure(L"Failed call to CreateThread!");
		goto cleanup;
	}
	
	// Wait on the thread
	wprintf(L"[INFO] Waiting for thread completion..\n");
	WaitForSingleObject(hThread, INFINITE);
	if(!GetExitCodeThread(hThread, &dwExitCode)) {
		failure(L"Failed call to GetExitCodeThread!");
		goto cleanup;
	}
	
	// Finally, check the thread exit code for success status, then cleanup
	// and return.
	wprintf(L"[INFO] Thread completed. Checking resulting exit code..\n");
	bResult = dwExitCode != 0;
	
cleanup:
	if(hThread != NULL) {
		CloseHandle(hThread);
		hThread = NULL;
	}
	
	if(pmem != NULL) {
		VirtualFreeEx(hProcess, pmem, size, MEM_RELEASE);
		pmem = NULL;
	}
	
	return bResult;
}

int wmain(int argc, wchar_t *argv[])
{
	void* *pdll;
	HANDLE hProcess = NULL;
	uint32_t* ploadlib = NULL;
	size_t codesize = INJECT_TEMPLATE_SIZE + TESTDLL_NAME_SIZE;
	uint8_t codebuffer[INJECT_TEMPLATE_SIZE + TESTDLL_NAME_SIZE] = {0};
	hProcess = GetCurrentProcess();
	
	wprintf(L"INFO: Building injection code..\n");
	RtlCopyMemory((void*)codebuffer, (const void*)INJECT_TEMPLATE, INJECT_TEMPLATE_SIZE);
	pdll = (void*)&(codebuffer[INJECT_TEMPLATE_SIZE]);
	ploadlib = (uint32_t*)&(codebuffer[INJECT_TEMPLATE_OFFSET_0]);
	RtlCopyMemory(pdll, (const void*)TESTDLL_NAME, TESTDLL_NAME_SIZE);
	*ploadlib = get_loadlib_rva();
	if(inject_thread(hProcess, (const void*)codebuffer, codesize)) {
		wprintf(L"[INFO] Success!\n");
	} else {
		wprintf(L"[INFO] Failure!\n");
	}
	CloseHandle(hProcess);
	return 0;
}

#ifdef _WINDOWS

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpOriginalCmdLine, int nCmdShow)
{
	if(!AllocConsole()) {
		
	}
	return wmain(__argc, __wargv);
}

#endif