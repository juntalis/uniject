/**
 * @file shared.c
 * Misc shared functionality used by both the injector and loader dlls.
 */
#include "pch.h"
#include <uniject/utility.h>
#include <uniject/win32.h>

#include <intrin.h>
#pragma intrinsic(_InterlockedCompareExchangePointer)

#ifdef _DEBUG
#define UNIJ_HEAP_OPTS HEAP_GENERATE_EXCEPTIONS
#else
#define UNIJ_HEAP_OPTS 0
#endif

// Heap used for this library's allocations
static HANDLE uniject_heap_handle = NULL;

// One-time execution data for unij_acquire_default_privileges
static unij_once_t memory_initialized = UNIJ_ONCE_INIT;

static UNIJ_NOINLINE
BOOL CDECL unij_memory_init_once(void* pData)
{
	HANDLE heap = GetProcessHeap();
	if(IS_INVALID_HANDLE(heap)) {
		heap = HeapCreate(UNIJ_HEAP_OPTS, 0, 0);
		if(IS_INVALID_HANDLE(heap)) {
			unij_fatal_call(HeapCreate);
			return FALSE;
		}
	}
	
	// Set the global and return
	uniject_heap_handle = heap;
	return TRUE;
}

bool unij_memory_init(void)
{
	return unij_once(&memory_initialized, unij_memory_init_once, NULL) && uniject_heap_handle != NULL;
}

void* unij_alloc(size_t size)
{
	return unij_memory_init() ? (void*)HeapAlloc(uniject_heap_handle, HEAP_ZERO_MEMORY, (SIZE_T)size) : NULL;
}

void unij_free(void* ptr)
{
	BOOL free_result;
	if(ptr == NULL) return;
	if(!unij_memory_init()) return;
	free_result = HeapFree(uniject_heap_handle, 0, ptr); 
	assert(free_result);
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
	RtlCopyMemory((void*)pResult, (const void*)src, count * sizeof(wchar_t));
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

unij_wstr_t unij_wstrdup(const unij_wstr_t* str)
{
	unij_wstr_t result = { 0, NULL };
	size_t length = (size_t)unij_wstrlen(str);
	if(length > 0) {
		result.length = (uint16_t)length;
		result.value = unij_wcsalloc(length);
		RtlCopyMemory((void*)result.value, (const void*)str->value, WSIZE(length));
	}
	return result;
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

const char* unij_prefix_vsacprintf(const char* prefix, const char* format, va_list args)
{
	size_t szPrefix = 0, szFormatted = 0;
	char* pWorking = NULL, *pResult = NULL;
	va_list argscopy;
	
	// Check the resulting size of the buffer.
	va_copy(argscopy, args);
	szFormatted = (size_t)_vscprintf(format, argscopy) + 1;
	va_end(argscopy);
	
	// Add additional space for the prefix
	if(prefix != NULL) {
		szPrefix = (size_t)lstrlenA(prefix);
	}
	
	// Allocate our buffer.
	pResult = (char*)unij_alloc(szFormatted + szPrefix);
	if(pResult == NULL) {
		return NULL;
	}

	// Finally, fill in the message.
	if(IS_VALID_STRING(prefix))
		lstrcpynA(pResult, prefix, (int)szPrefix + 1);
	
	pWorking = &(pResult[szPrefix]);
	_vsnprintf(pWorking, szFormatted, format, args);
	return (const char*)pResult;
}

const char* unij_prefix_sacprintf(const char* prefix, const char* format, ...)
{
	va_list args;
	const char* pResult;
	va_start(args, format);
	pResult = unij_prefix_vsacprintf(prefix, format, args);
	va_end(args);
	return pResult;
}

unij_cstr_t unij_wstrtocstr(const unij_wstr_t* str)
{
	int bufsize;
	unij_cstr_t result = { 0, NULL };
	int length = (int)unij_wstrlen(str);
	if(length == 0) return result;
	
	bufsize = WideCharToMultiByte(CP_UTF8, 0, str->value, length, NULL, 0, NULL, NULL);
	if(!bufsize) {
		unij_fatal_call(WideCharToMultiByte);
		return result;
	}
	
	result.length = (uint16_t)length;
	result.value = (const char*)unij_alloc((size_t)bufsize + sizeof(char));
	if(result.value == NULL) {
		unij_fatal_alloc();
		return result;
	}
	
	if(!WideCharToMultiByte(CP_UTF8, 0, str->value, length, (char*)result.value, bufsize, NULL, NULL)) {
		unij_cstrfree(&result);
		unij_fatal_call(WideCharToMultiByte);
	}
	
	return result;
}

#define TOPTR(PTR) \
	((uintptr_t)(PTR))

#define APPLY_OFFSET(PTR,OFF) \
	(TOPTR(PTR) + TOPTR(OFF))

wchar_t* unij_strtolower(wchar_t* buffer, size_t length)
{
	union
	{
		uint16_t* p16;
		uint32_t* p32;
		uint64_t* p64;
	} idata;
	idata.p16 = (uint16_t*)buffer;
	uintptr_t pend = APPLY_OFFSET(buffer, WSIZE(length));
	
	while(TOPTR(idata.p64) + sizeof(uint64_t) <= pend)
		*idata.p64++ |= (uint64_t)0x0020002000200020;
	
	
	while(TOPTR(idata.p32) + sizeof(uint32_t) <= pend)
		*idata.p32++ |= (uint32_t)0x00200020;
	 
	while(TOPTR(idata.p16) + sizeof(uint16_t) <= pend)
		*idata.p16++ |= (uint16_t)0x0020;
	
	return buffer;
}

