/**
 * @file process.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "peutil.h"

#include <tlhelp32.h>
#include <uniject/logger.h>
#include <uniject/process.h>
#include <uniject/module.h>

typedef struct unity_process_internal unity_process_internal_t;

static bool is_wow64_process(HANDLE hProcess)
{
	BOOL wow = FALSE;
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
	return fnIsWow64Process(hProcess, &wow) && wow;
}

static bool check_process_header(LPPROCESS_INFORMATION ppi, PMEMORY_BASIC_INFORMATION pminfo,
                                 PIMAGE_NT_HEADERS pnt_header)
{
	PBYTE pread;
	IMAGE_DOS_HEADER dos_header;
	if(pminfo->BaseAddress != pminfo->AllocationBase)
		return false;
	
	if(!READ_PROC_VAR(pminfo->BaseAddress, &dos_header) || (dos_header.e_magic != IMAGE_DOS_SIGNATURE))
		return false;
	
	pread = (PBYTE)pminfo->BaseAddress + dos_header.e_lfanew;
	if(!READ_PROC_VAR(pread, pnt_header) || (pnt_header->Signature != IMAGE_NT_SIGNATURE))
		return false;
	
	return pnt_header->FileHeader.Characteristics & IMAGE_FILE_DLL ? false : true;
}

unij_process_flags_t unij_get_process_type(LPPROCESS_INFORMATION ppi, uint32_t* pBase)
{
	PBYTE ptr;
	PBYTE dummy_base;
	IMAGE_NT_HEADERS nt_header;
	MEMORY_BASIC_INFORMATION minfo;
	if(pBase == NULL) pBase = &dummy_base;
	
	*pBase = NULL;
	for(ptr = NULL; VirtualQueryEx(ppi->hProcess, ptr, &minfo, sizeof(minfo)); ptr += minfo.RegionSize) {
		unij_process_flags_t result = UNIJ_PROCESS_INVALID;
		if(check_process_header(ppi, &minfo, &nt_header)) {
			*pBase = minfo.BaseAddress;
			if(nt_header.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
				result |= UNIJ_PROCESS_GUI;
			else if(nt_header.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
				result |= UNIJ_PROCESS_CUI;
			
			if((result & UNIJ_PROCESS_UI_MASK) == UNIJ_PROCESS_INVALID) {
				if(nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
					PIMAGE_NT_HEADERS32 pNTHeader = (PIMAGE_NT_HEADERS32)&nt_header;
					result |= UNIJ_PROCESS_32;
					if(is_wow64_process(ppi->hProcess))
						result |= UNIJ_PROCESS_64;
					return result;
				} else if(nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
					PIMAGE_NT_HEADERS32 pNTHeader = (PIMAGE_NT_HEADERS32)&nt_header;
					result |= UNIJ_PROCESS_64;
					return result;
				} else {
					LogDebug(log, "  Ignoring unsupported machine (0x%X)", nt_header.FileHeader.Machine);
				}
			} else {
				LogDebug(log, "  Ignoring unsupported subsystem (%u)", nt_header.OptionalHeader.Subsystem);
			}
		}
	}

	return UNIJ_PROCESS_INVALID;
}

static bool enum_process_modules(uint32_t pid, unij_unity_process_t* pprocess, HANDLE snapshot,
	                             unij_enum_process_fn fn, void* parameter)
{
	bool handled = false;
	BOOL ok = TRUE;
	MODULEENTRY32W me = { sizeof(me)} ;
	for(ok = Module32FirstW(snapshot, &me); ok; ok = Module32NextW(snapshot, &me)) {
		uint32_t rva = unij_get_proc_rva(me.szExePath, "mono_init");
		if(rva > 0) {
			pprocess->mono_name = (const wchar_t*)me.szModule;
			handled = fn(pprocess, parameter);
			break;
		}
	}
	return handled;
}

void unij_enum_unity_processes(unij_enum_process_fn fn, void* parameter)
{
	BOOL ok;
	HANDLE snapshot;
	PROCESSENTRY32W pe = { sizeof(pe) };
	uint32_t pid = GetCurrentProcessId();
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALLMODULES, pid);
	for(ok = Process32First( snapshot, &pe ); ok; ok = Process32Next( snapshot, &pe )) {
		bool handled = false;
		unij_unity_process_t process = { pe.th32ProcessID, (const wchar_t*)pe.szExeFile };
		if(pe.th32ProcessID == pid) {
			handled = enum_process_modules(pid, snapshot, &process, fn, parameter);
		} else {
			HANDLE snapshot2 = CreateToolhelp32Snapshot(TH32CS_SNAPALLMODULES, pe.th32ProcessID);
			handled = enum_process_modules(pe.th32ProcessID, &process, snapshot2, fn, parameter);
			CloseHandle(snapshot2);
		}
		if(handled) break;
	}
	CloseHandle(snapshot);
}
