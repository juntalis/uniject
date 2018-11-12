/**
 * @file module.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "peutil.h"
#include <uniject/module.h>
#include <uniject/error.h>
#include <uniject/logger.h>
#include <uniject/win32.h>

#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#	define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x20
#endif

#ifndef LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE
#	define LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE 0x20
#endif

#ifndef GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
#	define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 0x00000002
#endif

// Would normally try to use the long (> MAX_PATH) paths for Windows stuff, but I believe the system directory is
// mandated to fit within a MAX_PATH-sized buffer.

static wchar_t system32_dir_buffer[MAX_PATH] = L"";
static unij_wstr_t system32_dir = {
	0, (const wchar_t*)system32_dir_buffer
};

#ifdef UNIJ_ARCH_X64
static wchar_t syswow64_dir_buffer[MAX_PATH] = L"";
static unij_wstr_t syswow64_dir = {
	0, (const wchar_t*)syswow64_dir_buffer
};
#endif

// One-time execution data for unij_acquire_default_privileges
static unij_once_t module_initialized = UNIJ_ONCE_INIT;

static UNIJ_NOINLINE
BOOL CDECL unij_module_init_one(void* unused)
{
	system32_dir.length = (uint16_t)GetSystemDirectoryW(system32_dir_buffer, MAX_PATH);
#	ifdef UNIJ_ARCH_X64
	syswow64_dir.length = (uint16_t)GetSystemWow64DirectoryW(syswow64_dir_buffer, MAX_PATH);
#	endif
	return (system32_dir.length < MAX_PATH)
#	ifdef UNIJ_ARCH_X64
	    && (syswow64_dir.length < MAX_PATH);
#	endif
}

bool unij_module_init(void)
{
	return unij_once(&module_initialized, unij_module_init_one, NULL) ;
}

HMODULE unij_noref_module_ex(const wchar_t* pModuleName, uint32_t dwFlags)
{
	HMODULE hModule = NULL;
	if(!(dwFlags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
		dwFlags |= GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
	return GetModuleHandleExW(dwFlags, pModuleName, &hModule) ? hModule : NULL;
}

unij_wstr_t unij_system32_dir(void)
{
	return system32_dir;
}

#ifdef UNIJ_ARCH_X64

unij_wstr_t unij_syswow64_dir(void)
{
	return syswow64_dir;
}

#endif

bool unij_system_module_path(wchar_t* buffer, uint32_t* psize, const wchar_t* module, int bits)
{
	uint32_t length, modlen, size = *psize;
	memset(buffer, 0, WSIZE(*psize));
	SetLastError(ERROR_SUCCESS);
#	ifdef UNIJ_ARCH_X64
	if(bits == 32)
		length = GetSystemWow64DirectoryW(buffer, *psize);
	else
#	endif
		length = GetSystemDirectoryW(buffer, *psize);
	
	if((length+1) > *psize)
		return false;
	
	modlen = (uint32_t)lstrlenW(module);
	if((length + modlen + 2) > *psize) {
		return false;
	}
	buffer[length++] = L'\\';
	lstrcpynW(&buffer[length], module, modlen+1);
	return true;
}

uint32_t unij_get_proc_rva(const wchar_t* module, const char* proc)
{
	DWORD rva = 0;
	HMODULE module_handle = NULL;
	
	PWORD pordinals;
	PDWORD pfunctions, pnames;
	PIMAGE_NT_HEADERS pnt_hdr;
	PIMAGE_DOS_HEADER pdos_hdr;
	PIMAGE_EXPORT_DIRECTORY pexp_dir;
	int low = 0, mid = 0, high, cmp;
	
	module_handle = LoadLibraryEx(module, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if(module_handle == NULL) {
		unij_fatal_call(LoadLibraryEx);
		return 0;
	}
	
	pdos_hdr = (PIMAGE_DOS_HEADER)((DWORD_PTR)module_handle & ~0xFFFF);
	pnt_hdr = MAKE_VA(PIMAGE_NT_HEADERS, pdos_hdr, pdos_hdr->e_lfanew);

	if(pnt_hdr-> HDRMAGIC == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
#	ifdef UNIJ_ARCH_X64
		pexp_dir = MAKE_VA(PIMAGE_EXPORT_DIRECTORY, pdos_hdr, AS_NT_HEADERS32(pnt_hdr)-> EXPORTDIR .VirtualAddress);
	} else if(pnt_hdr-> HDRMAGIC == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
#	endif
		pexp_dir = MAKE_VA(PIMAGE_EXPORT_DIRECTORY, pdos_hdr, pnt_hdr-> EXPORTDIR .VirtualAddress);
	} else {
		FreeLibrary(module_handle);
		LogError(L"Unknown magic found in PE header: %hu", pnt_hdr-> HDRMAGIC);
		return 0;
	}
	
	// Grab a reference to the relevant tables in the export directory
	pnames = MAKE_VA(PDWORD, pdos_hdr, pexp_dir->AddressOfNames);
	pfunctions = MAKE_VA(PDWORD, pdos_hdr, pexp_dir->AddressOfFunctions);
	pordinals = MAKE_VA(PWORD, pdos_hdr, pexp_dir->AddressOfNameOrdinals);
	
	high = (int)(pexp_dir->NumberOfNames - 1);
	while(low <= high) {
		mid = (low + high) / 2;
		cmp = strcmp(proc, MAKE_VA(LPCSTR, pdos_hdr, pnames[mid]) );
		if(cmp == 0) {
			rva = pfunctions[pordinals[mid]];
			break;
		}
		
		if(cmp < 0)
			high = mid - 1;
		else
			low = mid + 1;
	}
	
	FreeLibrary(module_handle);
	return rva;
}


