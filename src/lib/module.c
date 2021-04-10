/**
 * @file module.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "peutil.h"
#include <uniject/module.h>
#include <uniject/logger.h>
#include <uniject/win32.h>

#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#	define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x20
#endif

#ifndef GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
#	define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 0x00000002
#endif

HMODULE unij_noref_module_ex(const wchar_t* module, uint32_t flags)
{
	HMODULE hModule = NULL;
	if(!(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
		flags |= GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
	return GetModuleHandleExW(flags, module, &hModule) ? hModule : NULL;
}

bool unij_system_module_path(wchar_t* buffer, uint32_t* psize, const wchar_t* module, int bits)
{
	uint32_t length, modlen, buflen = *psize;
	RtlZeroMemory((void*)buffer, (size_t)WSIZE(buflen));

#	ifdef UNIJ_ARCH_X64
	if(bits == 32)
		length = GetSystemWow64DirectoryW(buffer, buflen);
	else
#	endif
		length = GetSystemDirectoryW(buffer, buflen);
	
	if((length+1) > buflen)
		return false;
	
	modlen = (uint32_t)lstrlenW(module);
	if((length + modlen + 2) > buflen) {
		return false;
	}
	buffer[length++] = L'\\';
	lstrcpynW(&buffer[length], module, modlen+1);
	*psize = length + modlen;
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
	
	module_handle = LoadLibraryExW(module, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if(module_handle == NULL) {
		// unij_fatal_call(LoadLibraryEx);
		return 0;
	}
	
	pdos_hdr = AS_DOS_HEADER((DWORD_PTR)module_handle & ~0xFFFF);
	pnt_hdr = MAKE_VA(PIMAGE_NT_HEADERS, pdos_hdr, pdos_hdr->e_lfanew);

	if(pnt_hdr-> HDRMAGIC == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
#	ifdef UNIJ_ARCH_X64
		pexp_dir = MAKE_VA(PIMAGE_EXPORT_DIRECTORY, pdos_hdr, AS_NT_HEADERS32(pnt_hdr)-> EXPORTDIR .VirtualAddress);
	} else if(pnt_hdr-> HDRMAGIC == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
#	endif
		pexp_dir = MAKE_VA(PIMAGE_EXPORT_DIRECTORY, pdos_hdr, pnt_hdr-> EXPORTDIR .VirtualAddress);
	} else {
		FreeLibrary(module_handle);
		LogWarning(L"Unknown magic found in PE header: %hu", pnt_hdr-> HDRMAGIC);
		return 0;
	}
	
	
	if(pexp_dir->AddressOfNames != 0 && pexp_dir->AddressOfFunctions != 0 && pexp_dir->AddressOfNameOrdinals != 0) {
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
	}
	if(rva == 0) {
		LogWarning(L"Could not locate required export '%S' in module: %s", proc, module);
	}
	return rva;
}


