/**
 * @file peutil.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for peutil.h
 */
#ifndef _PEUTIL_H_
#define _PEUTIL_H_
#pragma once

#include <uniject.h>
#include <tlhelp32.h>

#define AS_UPTR(X)             ((uintptr_t)(X))
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

#define TH32CS_SNAPMODULES (TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32)
#define TH32CS_SNAPALLMODULES (TH32CS_SNAPPROCESS | TH32CS_SNAPMODULES)

// Reduce the verbosity of some functions (assuming variable names).
#define READ_PROC_VAR(P,A,B) READ_PROC_MEM(P, A, B, sizeof(*(B)) )
#define WRITE_PROC_VAR(P,A,B) WRITE_PROC_VAR(P, A, B, sizeof(*(B)) )
#define READ_PROC_MEM(P,A,B,C) ReadProcessMemory(P, A, B, C, NULL )
#define WRITE_PROC_MEM(P,A,B,C) WriteProcessMemory(P, A, B, C, NULL )
#define PROTECT_PROC_VAR(P,A,B,C) VirtualProtectEx(P, A, sizeof(*(A)), B, C)

// Macros for adding pointers/offsets together without C arithmetic interfering

#define MAKE_VA(TYPE,DOSHDR,OFFSET) \
	((TYPE)((DWORD_PTR)DOSHDR + (DWORD)(OFFSET)))

#endif /* _PEUTIL_H_ */
