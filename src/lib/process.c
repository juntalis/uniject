/**
 * @file process.c
 * @author Charles Grunwald <ch@rles.rocks>
 * 
 * TODO: Most Unity executables export NvOptimusEnablement/AmdPowerXpressRequestHighPerformance. Might be worth adding
 *       a first pass to the detection logic to check for that.
 */
#include "pch.h"
#include "process_private.h"

#include <uniject/module.h>
#include <uniject/logger.h>
#include <uniject/utility.h>
#include <uniject/win32.h>

static bool is_wow64_process(HANDLE hProcess)
{
	BOOL iswow64 = FALSE;
	static BOOL(WINAPI* fnIsWow64Process)(HANDLE, PBOOL) = NULL;
	
	if(fnIsWow64Process == INVALID_HANDLE_VALUE)
		return false;
	
	if(fnIsWow64Process == NULL) {
		HMODULE kernel32 = unij_noref_module(L"kernel32");
		ASSERT_NOT_NULL(kernel32);
		*(FARPROC*)(&fnIsWow64Process) = GetProcAddress(kernel32, "IsWow64Process");
		if(fnIsWow64Process == NULL) {
			fnIsWow64Process = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	
	// If IsWow64Process fails, say it is 64, since injection probably wouldn't
	// work, either.
	return fnIsWow64Process(hProcess, &iswow64) && iswow64;
}

static bool check_process_header(HANDLE process, PMEMORY_BASIC_INFORMATION pminfo,
                                 PIMAGE_NT_HEADERS pnt_header)
{
	PBYTE pread;
	IMAGE_DOS_HEADER dos_header;
	if(pminfo->BaseAddress != pminfo->AllocationBase)
		return false;
	
	if(!READ_PROC_VAR(process, pminfo->BaseAddress, &dos_header) || (dos_header.e_magic != IMAGE_DOS_SIGNATURE))
		return false;
	
	pread = (PBYTE)pminfo->BaseAddress + dos_header.e_lfanew;
	if(!READ_PROC_VAR(process, pread, pnt_header) || (pnt_header->Signature != IMAGE_NT_SIGNATURE))
		return false;
	
	return !(pnt_header->FileHeader.Characteristics & IMAGE_FILE_DLL);
}

static unij_procflags_t get_process_flags(HANDLE process)
{
	PBYTE ptr;
	IMAGE_NT_HEADERS nt_header;
	MEMORY_BASIC_INFORMATION minfo;
	
	for(ptr = NULL; VirtualQueryEx(process, ptr, &minfo, sizeof(minfo)); ptr += minfo.RegionSize) {
		unij_procflags_t result = UNIJ_PROCESS_INVALID;
		if(check_process_header(process, &minfo, &nt_header)) {
			if(nt_header.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
				result |= UNIJ_PROCESS_GUI;
			else if(nt_header.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
				result |= UNIJ_PROCESS_CUI;
			
			if((result & UNIJ_PROCESS_UI_MASK) != UNIJ_PROCESS_INVALID) {
				if(nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
					return result | UNIJ_PROCESS_WIN32;
				} else if(nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
					return result | UNIJ_PROCESS_WIN64;
				} else {
					LogDebug("Ignoring unsupported machine (0x%X)", nt_header.FileHeader.Machine);
				}
			} else {
				LogDebug("Ignoring unsupported subsystem (%u)", nt_header.OptionalHeader.Subsystem);
			}
		}
	}
	
	return UNIJ_PROCESS_INVALID;
}

// TODO: Cache previously checked modules
static bool enum_process_modules(unij_monoinfo_t* info, HANDLE snapshot, unij_monoinfo_fn fn, void* parameter)
{
	BOOL ok;
	bool handled = false;
	MODULEENTRY32W me = { sizeof(me)} ;
	for(ok = Module32FirstW(snapshot, &me); ok; ok = Module32NextW(snapshot, &me)) {
		uint32_t rva = unij_get_proc_rva(me.szExePath, "mono_init");
		if(rva > 0) {
			info->mono_name = (const wchar_t*)me.szModule;
			info->mono_path = (const wchar_t*)me.szExePath;
			handled = fn(info, parameter);
			break;
		}
	}
	return handled;
}

void unij_enum_mono_processes(unij_monoinfo_fn fn, void* parameter)
{
	BOOL ok;
	HANDLE snapshot;
	bool handled = false;
	PROCESSENTRY32W pe = { sizeof(pe) };
	uint32_t pid = GetCurrentProcessId();
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALLMODULES, pid);
	for(ok = Process32First( snapshot, &pe ); ok && !handled; ok = Process32Next( snapshot, &pe )) {
		unij_monoinfo_t info = { pe.th32ProcessID, (const wchar_t*)pe.szExeFile };
		if(pe.th32ProcessID == pid) {
			handled = enum_process_modules(&info, snapshot, fn, parameter);
		} else {
			HANDLE snapshot2 = CreateToolhelp32Snapshot(TH32CS_SNAPMODULES, pe.th32ProcessID);
			handled = enum_process_modules(&info, snapshot2, fn, parameter);
			CloseHandle(snapshot2);
		}
	}
	CloseHandle(snapshot);
}

static UNIJ_NOINLINE bool CDECL mono_path_resolver(unij_monoinfo_t* info, unij_wstr_t* dest)
{
	bool result;
	dest->length = (uint16_t)lstrlenW(info->mono_path);
	result = dest->length > 0;
	if(result) {
		dest->value = (const wchar_t*)unij_wcsndup(info->mono_path, (size_t)dest->length);
	}
	return result;
}

static bool resolve_mono_name(unij_wstr_t* dest, uint32_t pid)
{
	bool result;
	void* parameter = (void*)dest;
	unij_monoinfo_t info = { pid, NULL };
	unij_monoinfo_fn fn = (unij_monoinfo_fn)mono_path_resolver;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULES, pid);
	result = enum_process_modules(&info, snapshot, fn, parameter);
	CloseHandle(snapshot);
	return result;
}

static unij_process_t* process_assign_internal(HANDLE process)
{
	uint32_t pid;
	unij_procflags_t flags;
	unij_process_t* result = NULL;
	//unij_wstr_t mono_path = { 0, NULL };
	
	// Grab the process id. (More for the purpose of verifying that this is a real accessible process handle)
	pid = GetProcessId(process);
	if(pid == 0) {
		unij_fatal_error(UNIJ_ERROR_PROCESS, L"Invalid process handle passed to unij_process_assign");
		return result;
	}
	
	// Get the process flags - verify result
	flags = get_process_flags(process);
	if((flags & UNIJ_PROCESS_BITS_MASK) == UNIJ_PROCESS_INVALID) {
		unij_fatal_error(UNIJ_ERROR_PROCESS, L"Invalid process type passed to unij_process_assign");
		return result;
	}
	
	// Auto-resolve the filepath for the process's mono dll.
	//if(!resolve_mono_name(&mono_path, pid)) {
	//	unij_fatal_error(UNIJ_ERROR_PROCESS, L"No mono dll detected in process passed to unij_process_assign");
	//	return result;
	//}
	
	// Finally: allocate our result
	result = (unij_process_t*)unij_alloc(sizeof(*result));
	if(result == NULL) {
		//unij_free((void*)mono_path.value);
		//mono_path.value = NULL;
		unij_fatal_alloc();
		return result;
	}
	
	// Populate result & return
	result->pid = pid;
	result->flags = flags;
	result->process = process;
	//result->mono_path = mono_path;
	result->mono_path = UNIJ_EMPTY_WSTR;
	return result;
}

unij_process_t* unij_process_open(uint32_t pid)
{
	unij_process_t* result;
	HANDLE process;
	
	if(!unij_acquire_default_privileges()) {
		unij_fatal_call(unij_acquire_default_privileges);
		return NULL;
	}
	
	// Open process handle
	if(pid == 0) {
		process = GetCurrentProcess();
	} else {
		process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	}
	
	// Verify result
	if(IS_INVALID_HANDLE(process)) {
		unij_fatal_call(OpenProcess);
		return NULL;
	}
	
	// Check result of process_assign_internal. On failure, close the opened handle
	result = process_assign_internal(process);
	if(result == NULL) {
		CloseHandle(process);
	}
	
	return result;
}

unij_process_t* unij_process_assign(HANDLE process)
{
	HANDLE current_process, duplicate = NULL;
	unij_process_t* result = NULL;
	
	// Verify process handle
	if(IS_INVALID_HANDLE(process)) {
		unij_fatal_null(process);
		return result;
	}
	
	// Attempt to duplicate the handle.
	current_process = GetCurrentProcess();
	if(!DuplicateHandle(current_process, process, current_process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		unij_fatal_call(DuplicateHandle);
		return result;
	}
	
	// Check result of process_assign_internal. On failure, close the duplicated handle
	result = process_assign_internal(process);
	if(result == NULL) {
		CloseHandle(process);
	}
	
	return result;
}

void unij_process_close(unij_process_t* process)
{
	if(process == NULL) return;
	
	// Close the process handle
	if(IS_VALID_HANDLE(process->process)) {
		CloseHandle(process->process);
		process->process = NULL;
	}
	
	// Free our mono path
	unij_wstrfree(&process->mono_path);
	unij_free(process);
}

unij_procflags_t unij_process_flags(unij_process_t* process, unij_procflags_t mask)
{
	if(unij_fatal_null(process))
		return UNIJ_PROCESS_INVALID;
	if(mask == UNIJ_PROCESS_INVALID)
		mask = UNIJ_PROCESS_MASK;
	return process->flags & mask;
}

uint32_t unij_process_get_pid(unij_process_t* process)
{
	return unij_fatal_null(process) ? 0 : process->pid;
}

HANDLE unij_process_get_handle(unij_process_t* process)
{
	return unij_fatal_null(process) ? NULL : process->process;
}

unij_wstr_t* unij_process_get_mono_path(unij_process_t* process)
{
	unij_wstr_t* mono_path;
	if(unij_fatal_null(process))
		return NULL;
	
	mono_path = &process->mono_path;
	if(unij_is_empty(mono_path)) {
		if(!resolve_mono_name(mono_path, process->pid)) {
			unij_fatal_error(UNIJ_ERROR_PROCESS, L"No mono dll detected in currently selected process!");
			return NULL;
		}
	}
	
	return mono_path;
}
