/**
 * @file injector.c
 * @author Charles Grunwald <ch@rles.rocks>
 * 
 * TODO: Thread hijacking doesn't need to be done with shellcode/VirtualAlloc. I used that approach, because I had
 *       suitable code laying around that I could reuse. The better approach would be to rewrite the logic in MASM,
 *       link the object file in, and wrap the execution in a critical section to avoid race conditions. This will
 *       deal with the current memory leak that occurs when we allocate but don't free up memory for the shellcode.
 *       
 *       Main issue is figuring out how to return to the original eip while preserving the original context. Few
 *       ideas - just need to write some test code and see which works.
 */
#include "pch.h"
#include "process_private.h"

#include <conio.h>
#include <uniject/injector.h>
#include <uniject/module.h>
#include <uniject/utility.h>


#define FILE_ATTRIBUTE_NOTFILE \
	(FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_DIRECTORY)

typedef struct inject_params inject_params_t;
typedef struct hijack_params hijack_params_t;
typedef struct callback_data callback_data_t;
typedef struct template_code template_code_t;

struct template_code
{
	uint32_t param_off;
	uint32_t entrypoint_off;
	
	size_t template_size;
	const uint8_t* template;
};

struct inject_params
{
	unij_wstr_t* loader;
	unij_process_t* process;
	uint32_t loadlib_rva;
	const template_code_t* code;
	size_t code_size;
};

struct callback_data
{
	uintptr_t callback_fn;
	uintptr_t callback_param;
};

struct hijack_params
{
	uintptr_t original_ip;
	callback_data_t callback;
	const template_code_t* code;
	size_t code_size;
};

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

// Template offset/size data
#define INJECT_TEMPLATE32_OFFSET_0   0x26
#define INJECT_TEMPLATE32_ENTRYPOINT 0x00
#define INJECT_TEMPLATE32_SIZE       sizeof(INJECT_TEMPLATE32)

#define INJECT_TEMPLATE64_OFFSET_0   0x22
#define INJECT_TEMPLATE64_ENTRYPOINT 0x00
#define INJECT_TEMPLATE64_SIZE       sizeof(INJECT_TEMPLATE64)

// Code descriptors
static const template_code_t INJECT_CODE32 = {
	INJECT_TEMPLATE32_OFFSET_0,
	INJECT_TEMPLATE32_ENTRYPOINT,
	INJECT_TEMPLATE32_SIZE,
	INJECT_TEMPLATE32
};

static const template_code_t INJECT_CODE64 = {
	INJECT_TEMPLATE64_OFFSET_0,
	INJECT_TEMPLATE64_ENTRYPOINT,
	INJECT_TEMPLATE64_SIZE,
	INJECT_TEMPLATE64
};

#ifdef UNIJ_ARCH_X86

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

#define HIJACK_IP Eip
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

#define HIJACK_IP Rip
#define HIJACK_TEMPLATE_OFFSET_0 0x50
#define HIJACK_TEMPLATE_ENTRYPOINT 0x00

#endif

#define HIJACK_TEMPLATE_SIZE sizeof(HIJACK_TEMPLATE)

static const template_code_t HIJACK_CODE = {
	HIJACK_TEMPLATE_OFFSET_0,
	HIJACK_TEMPLATE_ENTRYPOINT,
	HIJACK_TEMPLATE_SIZE,
	HIJACK_TEMPLATE
};

static UNIJ_INLINE uint32_t get_loadlib_rva(int bits)
{
	uint32_t bufsize = MAX_PATH;
	wchar_t buffer[MAX_PATH+1] = EMPTY_STRINGW;
	if(!unij_system_module_path(buffer, &bufsize, L"kernel32.dll", bits)) {
		unij_fatal_call(unij_system_module_path);
		return 0;
	}
	
	return unij_get_proc_rva((const wchar_t*)buffer, "LoadLibraryW");
}

static bool get_loader_path(unij_wstr_t* path_buffer, unij_procflags_t flags)
{
	wchar_t* pname, *buffer = (wchar_t*)path_buffer->value;
	uint32_t buflen = (uint32_t)path_buffer->length,
	required_len = (uint32_t)ARRAYLEN(UNIJ_LOADER32_NAME),
	len = GetModuleFileNameW(NULL, buffer, buflen);
	if(len == 0) {
		unij_fatal_call(GetModuleFileNameW);
		return false;
	}
	 
	// Erase the executable filename, leaving only the parent folder.
	while(buffer[--len] != L'\\' && len)
		buffer[len] = L'\0';
	
	// Verify len
	if(len == 0) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Failure while trying to auto-resolve loader path");
		return false;
	}
	
	// Verify buffer size can hold the length required.
	pname = &(buffer[++len]);
	required_len += len;
	if(required_len > buflen) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Failure while trying to auto-resolve loader path: required length "
		                                      L"exceeds alotted buffer length");
		return false;
	}
	
	if(flags & UNIJ_PROCESS_WIN32) {
		lstrcpyW(pname, UNIJ_LOADER32_NAME);
	} else {
		lstrcpyW(pname, UNIJ_LOADER64_NAME);
	}
	
	path_buffer->length = (uint16_t)(required_len - 1);
	return true;
}

static void* render_inject_code(inject_params_t* params)
{
	void* procmem = NULL;
	wchar_t* ppath = NULL;
	uint32_t* prva = NULL;
	uint8_t* buffer = NULL;
	unij_wstr_t* loader = params->loader;
	unij_process_t* process = params->process;
	const template_code_t* code = params->code;
	size_t code_size = code->template_size + AS_UPTR(WSIZE(loader->length + 1));
	
	// First try allocating the memory we intend to inject in the target process.
	procmem = VirtualAllocEx(process->process, NULL, code_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(procmem == NULL) {
		unij_fatal_call(VirtualAllocEx);
		return NULL;
	}
	
	// Then allocate our local buffer for building the code
	buffer = (uint8_t*)unij_alloc(code_size);
	if(buffer == NULL) {
		unij_fatal_alloc();
		VirtualFreeEx(process->process, procmem, code_size, MEM_RELEASE);
		return NULL;
	}
	
	// Fill in our buffer with the shellcode template, then fill in the LoadLibraryW RVA
	RtlCopyMemory((void*)buffer, (const void*)code->template, code->template_size);
	prva = MAKE_PTR(uint32_t, buffer, code->param_off);
	*prva = params->loadlib_rva;
	
	// Fill in the loader dll path after the template code.
	ppath = MAKE_PTR(wchar_t, buffer, code->template_size);
	RtlCopyMemory((void*)ppath, (const void*)loader->value, AS_UPTR(WSIZE(loader->length)));
	
	// Finally, write the assembled code to our target process
	if(!WriteProcessMemory(process->process, procmem, buffer, code_size, NULL)) {
		unij_free((void*)buffer);
		VirtualFreeEx(process->process, procmem, code_size, MEM_RELEASE);
		unij_fatal_call(WriteProcessMemory);
		return 0;
	}
	
	// Flush the instruction cache and cleanup our buffer
	FlushInstructionCache(process->process, (const void*)procmem, code_size);
	unij_free((void*)buffer);
	params->code_size = code_size;
	return procmem;
}

// Hijack code's always gonna be a fixed size - no need to dynamically allocate a buffer for rendering.
#define HIJACK_CODE_SIZE (HIJACK_TEMPLATE_SIZE + sizeof(callback_data_t))

static void* render_hijack_code(hijack_params_t* params)
{
	HANDLE process;
	uintptr_t* pip;
	void* pcallback_data;
	void* procmem = NULL;
	size_t code_size = HIJACK_CODE_SIZE;
	uint8_t buffer[HIJACK_CODE_SIZE] = {0};
	const template_code_t* code = params->code;
	
	// We already need the process handle for our call to FlushInstructionCache, so might keep a local and use
	// the *Ex variants of the virtual memory API
	process = GetCurrentProcess();
	procmem = VirtualAllocEx(process, NULL, code_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(procmem == NULL) {
		unij_fatal_call(VirtualAllocEx);
		return NULL;
	}
	
	// Fill in our buffer with the shellcode template, then fill in the LoadLibraryW RVA
	RtlCopyMemory((void*)buffer, (const void*)code->template, code->template_size);
	
	// Original eip/rip
	pip = MAKE_PTR(uintptr_t, buffer, code->param_off);
	*pip = params->original_ip;
	
	// Callback data
	pcallback_data = MAKE_PTR(void, buffer, code->template_size);
	RtlCopyMemory(pcallback_data, (const void*)&(params->callback), sizeof(params->callback));
	
	// Finally, write the assembled code to our target process
	if(!WriteProcessMemory(process, procmem, buffer, code_size, NULL)) {
		unij_free((void*)buffer);
		VirtualFreeEx(process, procmem, code_size, MEM_RELEASE);
		unij_fatal_call(WriteProcessMemory);
		return 0;
	}
	
	// Flush the instruction cache and cleanup our buffer
	FlushInstructionCache(process, (const void*)procmem, code_size);
	params->code_size = code_size;
	return procmem;
}

#ifdef UNIJ_ARCH_X86
#	define HEXPTR_FORMAT L"%08X"
#else
#	define HEXPTR_FORMAT L"%016" UNIJ_WIDEN(PRIX64)
#endif

static bool execute_injection(inject_params_t* params, void* pmem)
{
	bool result = true;
	DWORD tid = 0, thread_exit = 0;
	void* entrypoint, *thparam;
	const template_code_t* code = params->code;
	HANDLE remote_thread, process = params->process->process;
	
	// Resolve the remote address of the loader dll
	thparam = MAKE_PTR(void, pmem, code->template_size);
	
	// Resolve the remote address of the injection code's entrypoint
	entrypoint = MAKE_PTR(void, pmem, code->entrypoint_off);
	
	// Create the remote thread & verify
	remote_thread = CreateRemoteThread(
		process,
		NULL, 0,
		(LPTHREAD_START_ROUTINE)entrypoint,
		thparam,
		CREATE_SUSPENDED, &tid
	);
	if(IS_INVALID_HANDLE(remote_thread)) {
		unij_fatal_call(CreateRemotethread);
		return false;
	}
	
	
	unij_show_message(UNIJ_LEVEL_INFO, L"Created suspended thread: %lu [%08X]", tid, tid);
	unij_show_message(UNIJ_LEVEL_INFO, L"  Entrypoint = " HEXPTR_FORMAT , entrypoint);
	unij_show_message(UNIJ_LEVEL_INFO, L"  Parameter = " HEXPTR_FORMAT , thparam);
	unij_show_message(UNIJ_LEVEL_INFO, L"Press any key to resume thread..");
	_getch();
	ResumeThread(remote_thread);
	
	// TODO: Possible non-blocking implementation
	WaitForSingleObject(remote_thread, INFINITE);
	if(!GetExitCodeThread(remote_thread, &thread_exit)) {
		result = false;
		unij_show_message(UNIJ_LEVEL_ERROR, L"Call to GetExitCodeThread failed!");
		unij_abort(UNIJ_ERROR_LASTERROR);
	} else if(thread_exit == 0) {
		result = false;
		unij_show_message(UNIJ_LEVEL_ERROR, L"Injected thread %lu exited with code 0!", tid);
		unij_abort(UNIJ_ERROR_INTERNAL);
	}
	CloseHandle(remote_thread);
	return result;
}

bool unij_inject_loader_ex(unij_process_t* process, unij_wstr_t* loader)
{
	int bits;
	bool result;
	uint32_t fileattrs;
	void* procmem = NULL;
	inject_params_t params = {NULL};
	const wchar_t fallback_buffer[MAX_PATH+1] = EMPTY_STRINGW;
	unij_wstr_t fallback = { MAX_PATH, fallback_buffer };
	
	// Verify the process
	if(unij_fatal_null(process))
		return false;
	
	// Verify the loader path as a string
	if(!unij_wstring(loader)) {
		// If not set, use the default loader path for this process type.
		if(!get_loader_path(&fallback, process->flags))
			return false;
		loader = &fallback;
	}
	
	// Verify the loader path as a file
	fileattrs = GetFileAttributesW(loader->value);
	if((fileattrs == INVALID_FILE_ATTRIBUTES) || (fileattrs & FILE_ATTRIBUTE_NOTFILE)) {
		unij_fatal_error(UNIJ_ERROR_LOADERS, L"%s does not point to a valid filepath!", loader->value);
		return false;
	}
	
	// Extract process architecture
	bits = unij_process_bits(process);
	
	// Initial params setup.
	params.loader = loader;
	params.process = process;
	
	// Select appropriate shellcode
	if(bits == 32) {
		params.code = &INJECT_CODE32;
#	ifdef UNIJ_ARCH_X64
	} else if(bits == 64) {
		params.code = &INJECT_CODE64;
#	endif
	} else {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Unsupported architecture detected! Process bits=%d", bits);
		return false;
	}
	
	// Lookup LoadLibraryW RVA
	params.loadlib_rva = get_loadlib_rva(bits);
	if(params.loadlib_rva == 0) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Failed to locate LoadLibraryW export!");
		return false;
	}
	
	// Render injection code
	procmem = render_inject_code(&params);
	if(procmem == NULL) {
		return false;
	}
	
	// Inject our remote thread
	unij_show_message(UNIJ_LEVEL_INFO, L"Attempting to inject loader DLL: %s", loader->value);
	result = execute_injection(&params, procmem);
	VirtualFreeEx(process->process, procmem, params.code_size, MEM_RELEASE);
	return result;
}

#define SUSPEND_FAILURE ((DWORD)-1)

bool unij_hijack_thread(HANDLE thread, unij_hijack_fn fn, void* param)
{
	HANDLE process;
	bool result = true;
	void* procmem = NULL;
	hijack_params_t params = { 0 };
	CONTEXT context = { CONTEXT_CONTROL, 0 };
	if(IS_INVALID_HANDLE(thread)) {
		unij_fatal_error(UNIJ_ERROR_PROCESS, L"Invalid thread handle passed in call to unij_hijack_thread!");
		return false;
	}
	
	// Attempt to suspend the thread
	if(SuspendThread(thread) == SUSPEND_FAILURE) {
		unij_fatal_call(SuspendThread);
		return false;
	}
	
	// Attempt to grab the thread's context
	if(!GetThreadContext(thread, &context)) {
		unij_fatal_call(GetThreadContext);
		result = false;
		goto hijack_complete;
	}
	
	// Populate our params struct
	params.code = &HIJACK_CODE;
	params.code_size = HIJACK_CODE_SIZE;
	params.callback.callback_fn = (uintptr_t)fn;
	params.callback.callback_param = (uintptr_t)param;
	params.original_ip = (uintptr_t)context. HIJACK_IP;
	
	// Render hijack code
	procmem = render_hijack_code(&params);
	if(procmem == NULL) {
		result = false;
		goto hijack_complete;
	}
	
	// Change the thread eip/rip. On failure, cleanup memory.
	context. HIJACK_IP = (uintptr_t)MAKE_PTR(void, procmem, params.code->entrypoint_off);
	if(!SetThreadContext(thread, &context)) {
		result = false;
		process = GetCurrentProcess();
		VirtualFreeEx(process, procmem, params.code_size, MEM_RELEASE);
		unij_fatal_call(SetThreadContext);
	}
	
hijack_complete:
	// Whether successful or not, if we suspended the thread, we need to resume it.
	ResumeThread(thread);
	
	return result;
}

bool unij_hijack_thread_id(uint32_t tid, unij_hijack_fn fn, void* param)
{
	bool result;
	HANDLE thread;
	if(tid == 0) {
		unij_fatal_error(UNIJ_ERROR_PROCESS, L"Invalid thread ID specified in call to unij_hijack_thread: 0");
		return false;
	} else if(unij_fatal_null(fn)) {
		return false;
	}
	
	thread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)tid);
	if(IS_INVALID_HANDLE(thread)) {
		unij_fatal_call(OpenThread);
		return false;
	}
	
	result = unij_hijack_thread(thread, fn, param);
	CloseHandle(thread);
	return result;
}
