/**
 * @file shared.c
 * Misc shared functionality used by both the injector and loader dlls.
 */
#include "pch.h"
#include <uniject/utility.h>
#include <uniject/error.h>
#include <uniject/win32.h>

#include <intrin.h>
#pragma intrinsic(_InterlockedCompareExchangePointer)

#define _SZ(PTR) \
	((size_t)(PTR))

#define _OFFSET(PTR,OFF) \
	(_SZ(PTR) + _SZ(OFF))

#ifdef _DEBUG
#define UNIJ_HEAP_OPTS HEAP_GENERATE_EXCEPTIONS
#else
#define UNIJ_HEAP_OPTS 0
#endif

union generic_buffer
{
	uint8_t*  p8;
	uint16_t* p16;
	uint32_t* p32;
	uint64_t* p64;
};

typedef union generic_buffer generic_buffer_t;

// Heap used for this library's allocations
static UNIJ_CACHE_ALIGN HANDLE uniject_heap_handle = NULL;

// One-time execution data for unij_acquire_default_privileges
static unij_once_t memory_initialized = UNIJ_ONCE_INIT;

static UNIJ_NOINLINE
BOOL CDECL unij_memory_init_once(void* pData)
{
	HANDLE hHeap =  HeapCreate(UNIJ_HEAP_OPTS, 0, UNIJ_HEAP_MAXSIZE);
	if(IS_INVALID_HANDLE(hHeap)) {
		unij_fatal_call(HeapCreate);
		return FALSE;
	}
	
	// Set the global and return
	uniject_heap_handle = hHeap;
	return TRUE;
}

bool unij_memory_init(void)
{
	return unij_once(&memory_initialized, unij_memory_init_once, NULL) && uniject_heap_handle != NULL;
}

void* unij_alloc(size_t size)
{
	bool has_heap = unij_memory_init();
	return has_heap ? (void*)HeapAlloc(uniject_heap_handle, HEAP_ZERO_MEMORY, (SIZE_T)size) : NULL;
}

void unij_free(void* ptr)
{
	bool has_heap;
	BOOL free_result;
	if(ptr == NULL) return;
	has_heap = unij_memory_init();
	if(!has_heap) return;
	free_result = HeapFree(uniject_heap_handle, 0, ptr); 
	assert(free_result);
}

bool unij_memory_shutdown(void)
{
	bool result = true;
	HANDLE hHeapPrevious = NULL;
	HANDLE hHeapCurrent = *((volatile HANDLE*)&uniject_heap_handle);
	// If the heap handle is NULL by the time we get to it... Don't worry about it? (assume another call was successful
	// or it was never initialized)
	hHeapPrevious = _InterlockedCompareExchangePointer(&uniject_heap_handle, NULL, hHeapCurrent);
	if(hHeapPrevious != NULL) {
		result = HeapDestroy(hHeapPrevious) ? true : false;
	}
	return result;
}

wchar_t* unij_wcsndup(const wchar_t* src, size_t count)
{
	size_t szDest;
	wchar_t* pResult;
	if(IS_INVALID_STRING(src)) {
		return NULL;
	}

	szDest = (size_t)lstrlenW(src);
	if(szDest < count) {
		count = szDest;
	}

	pResult = unij_wcsalloc(count + 1);
	if(pResult == NULL) unij_fatal_alloc();
	RtlCopyMemory((PVOID)pResult, (const PVOID)src, count * sizeof(wchar_t));
	return pResult;
}

wchar_t* unij_wcsdup(const wchar_t* src)
{
	size_t length;
	if(IS_INVALID_STRING(src)) {
		return NULL;
	}
	
	length = (size_t)lstrlenW(src);
	return unij_wcsndup(src, length);
}

const wchar_t* unij_prefix_vsawprintf(const wchar_t* prefix, const wchar_t* format, va_list args)
{
	size_t szPrefix = 0, szFormatted = 0;
	wchar_t* pWorking = NULL, *pResult = NULL;
	va_list argscopy;
	
	// Check the resulting size of the buffer.
	va_copy(argscopy, args);
	szFormatted = (size_t)_vscwprintf(format, argscopy) + 1;
	va_end(argscopy);
	
	// Add additional space for the prefix
	if(prefix != NULL) {
		szPrefix = (size_t)lstrlenW(prefix);
	}
	
	// Allocate our buffer.
	pResult = unij_wcsalloc(szFormatted + szPrefix);
	if(pResult == NULL) {
		return NULL;
	}

	// Finally, fill in the message.
	if(IS_VALID_STRING(prefix))
		lstrcpynW(pResult, prefix, (int)szPrefix + 1);
	
	pWorking = &(pResult[szPrefix]);
	_vsnwprintf(pWorking, szFormatted, format, args);
	return (const wchar_t*)pResult;
}

const wchar_t* unij_prefix_sawprintf(const wchar_t* prefix, const wchar_t* format, ...)
{
	va_list args;
	const wchar_t* pResult;
	va_start(args, format);
	pResult = unij_prefix_vsawprintf(prefix, format, args);
	va_end(args);
	return pResult;
}

wchar_t* unij_simplify_path(wchar_t* dest, const wchar_t* src, size_t length)
{
	generic_buffer_t isrc, idest;
	isrc.p8 = (uint8_t*)src;
	idest.p8 = (uint8_t*)dest;
	size_t pend, size = WSIZE(length);
	pend = _OFFSET(src, --size);
	
	while(_SZ(isrc.p64) < pend)
		*idest.p64++ = *isrc.p64++ | (uint64_t)0x0020002000200020;
	
	while(_SZ(isrc.p32) < pend)
		*idest.p32++ = *isrc.p32++ | (uint32_t)0x00200020;
	 
	while(_SZ(isrc.p16) < pend)
		*idest.p16++ = *isrc.p16++ | (uint16_t)0x0020;
	
	dest[(size/2)+1] = L'\0';
	return dest;
}
