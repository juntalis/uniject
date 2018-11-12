/**
 * @file pe_util.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for pe_util.h
 */
#ifndef _UNIJECT_PEUTIL_H_
#define _UNIJECT_PEUTIL_H_
#pragma once

#include <uniject.h>

#define AS_DOS_HEADER(PTR)     ((PIMAGE_DOS_HEADER)(PTR))
#define AS_NT_HEADERS32(PTR)   ((PIMAGE_NT_HEADERS32)(PTR))
#define AS_DATA_DIRECTORY(PTR) ((PIMAGE_DATA_DIRECTORY)(PTR))

#define HDRMAGIC  OptionalHeader.Magic
#define DATADIRS  OptionalHeader.NumberOfRvaAndSizes
#define EXPORTDIR OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
#define IMPORTDIR OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
#define BOUNDDIR  OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT]
#define IATDIR    OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]
#define COMDIR    OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR]

#ifndef TH32CS_SNAPMODULE32
#	define TH32CS_SNAPMODULE32 0x10
#endif

#define TH32CS_SNAPALLMODULES (TH32CS_SNAPPROCESS | TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32)

// Reduce the verbosity of some functions (assuming variable names).
#define READ_PROC_VAR(a, b) READ_PROC_MEM(a, b, sizeof(*(b)) )
#define WRITE_PROC_VAR(a, b) WRITE_PROC_VAR( a, b, sizeof(*(b)) )
#define READ_PROC_MEM(a, b, c) ReadProcessMemory( ppi->hProcess, a, b, c, NULL )
#define WRITE_PROC_MEM(a, b, c) WriteProcessMemory( ppi->hProcess, a, b, c, NULL )
#define PROTECT_PROC_VAR(a, b) VirtualProtectEx( ppi->hProcess, a, sizeof(*(a)), b, &pr )

// Macro for adding pointers/DWORDs together without C arithmetic interfering
#define MAKE_VA(CAST,DOS,OFFSET) (CAST)((DWORD_PTR)DOS + (DWORD)(OFFSET))

#endif /* _UNIJECT_PEUTIL_H_ */
