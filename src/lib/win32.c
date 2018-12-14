/**
 * @file win32_api.c
 * TODO: description
 */
#include "pch.h"
#include <uniject/win32.h>
#include <uniject/utility.h>

#include <intrin.h>
// TODO: Atomics compatibility macros
#pragma intrinsic(_InterlockedCompareExchangePointer)

// Privileges
#define LUID_SIZE(COUNT)  ((uint32_t)(sizeof(LUID_AND_ATTRIBUTES) * (COUNT)))
#define PRIVILEGES_OFFSET ((uint32_t)FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges))

// Private run once primitive declaration
struct unij_once_internal
{
	UNIJ_CACHE_ALIGN
	HANDLE event;
	BOOL executed;
	BOOL result;
};

typedef struct unij_once_internal unij_once_internal_t;

// One-time execution data for unij_acquire_default_privileges
static unij_once_t acquired_current_process_privileges = UNIJ_ONCE_INIT;

// Convert a size_t value to a ULARGE_INTEGER.
static UNIJ_INLINE ULARGE_INTEGER make_ularge_integer(size_t szValue)
{
	uint64_t ullTempStore = (uint64_t)szValue; 
	return *((PULARGE_INTEGER)(&ullTempStore));
}

static UNIJ_NOINLINE
BOOL CDECL unij_once_inner(unij_once_internal_t* pOnce, unij_once_fn fnOnce, void* pData)
{
	DWORD dwResult;
	HANDLE hExisting, hCreated;

	hCreated = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(IS_INVALID_HANDLE(hCreated)) {
		unij_fatal_call(CreateEvent);
		return FALSE;
	}
	
	hExisting = _InterlockedCompareExchangePointer(&pOnce->event, hCreated, NULL);
	if(IS_INVALID_HANDLE(hExisting)) {
		pOnce->result = fnOnce(pData);
		dwResult = SetEvent(hCreated) ? 1 : 0;
		assert(dwResult > 0);
		pOnce->executed = TRUE;
	} else {
		// Wait for event signal
		CloseHandle(hCreated);
		dwResult = WaitForSingleObject(hExisting, INFINITE);
		assert(dwResult == WAIT_OBJECT_0);
	}

	return pOnce->result;
}

bool unij_once(unij_once_t* once, unij_once_fn oncefn, void* parameter)
{
	BOOL bResult;
	/* Fast case - avoid WaitForSingleObject. */
	unij_once_internal_t* pOncePriv = (unij_once_internal_t*)once;
	bResult = pOncePriv->executed ? pOncePriv->result : unij_once_inner(pOncePriv, oncefn, parameter);
	return bResult ? true : false;
}

static UNIJ_INLINE
PTOKEN_PRIVILEGES unij_alloc_token_privileges(const wchar_t** names, uint32_t count, uint32_t* pbuffer_size)
{
	PTOKEN_PRIVILEGES privileges = NULL;
	*pbuffer_size = PRIVILEGES_OFFSET + LUID_SIZE(count);

	privileges = (PTOKEN_PRIVILEGES)unij_alloc((size_t) *pbuffer_size);
	if(privileges != NULL) {
		uint32_t idx = 0;
		privileges->PrivilegeCount = count;
		for(; idx < count; idx++) {
			if(!LookupPrivilegeValueW(NULL, names[idx], &privileges->Privileges[idx].Luid)) {
				unij_free((void*)privileges);
				privileges = NULL;
				break;
			}

			privileges->Privileges[idx].Attributes = SE_PRIVILEGE_ENABLED;
		}
	} else {
		// Shouldnt actually hit this
		unij_fatal_alloc();
		SetLastError(ERROR_OUTOFMEMORY);
	}

	return privileges;
}

bool unij_acquire_privileges(HANDLE process, const wchar_t** names, uint32_t count)
{
	HANDLE hToken = NULL;
	BOOL bResult = FALSE;

	if(OpenProcessToken(process, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken )) {
		uint32_t buffer_size = 0, last_error = ERROR_SUCCESS, i = 0;
		PTOKEN_PRIVILEGES lpPrivileges = unij_alloc_token_privileges(names, count, &buffer_size);

		// Mark the call as successful
		if(lpPrivileges != NULL) {
			bResult = AdjustTokenPrivileges(hToken, FALSE, lpPrivileges, buffer_size, NULL, NULL);
			if(bResult == FALSE)
				last_error = GetLastError();
			unij_free((void*)lpPrivileges);
		} else {
			last_error = GetLastError();
		}
		
		CloseHandle(hToken);
		SetLastError(last_error);
	}
	
	return bResult ? true : false;
}

static UNIJ_NOINLINE
BOOL CDECL unij_acquire_default_privileges_once(void* pData)
{
	static const wchar_t* pNames[] = UNIJ_SE_TOKENS;
	static uint32_t uNamesCount = (uint32_t)ARRAYLEN(pNames);
	return unij_acquire_privileges(GetCurrentProcess(), pNames, uNamesCount) ? TRUE : FALSE;
}

bool unij_acquire_default_privileges(void) 
{
	return unij_once(&acquired_current_process_privileges, unij_acquire_default_privileges_once, NULL);
}

static UNIJ_INLINE const wchar_t* unij_object_type_text(unij_object_t type)
{
	switch(type)
	{
		case UNIJ_OBJECT_MAPPING:
			return L"mem";
		case UNIJ_OBJECT_EVENT:
			return L"ev";
		default:
			return L"unknown";
	}
}

// TODO: pid is coming up as 0 in the loader dll call. Need to find out why
const wchar_t* unij_object_name(const wchar_t* key, unij_object_t type, uint32_t pid)
{
	const wchar_t* stype = unij_object_type_text(type);
	//ASSERT_NOT_ZERO(pid);
	ASSERT_VALID_STRING(key);
	return unij_sawprintf(UNIJ_OBJECT_FORMAT, key, stype);
	//return unij_sawprintf(UNIJ_OBJECT_FORMAT, key, stype, pid);
}

HANDLE unij_create_mmap(const wchar_t* name, size_t size)
{
	HANDLE hResult = NULL;
	ULARGE_INTEGER ullSize = *((PULARGE_INTEGER)(&size));
	
	ASSERT_NOT_ZERO(size);
	ASSERT_VALID_STRING(name);
	
	//  Create our file mapping.
	hResult = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, ullSize.HighPart, ullSize.LowPart, name);
	if(IS_INVALID_HANDLE(hResult)) {
		unij_fatal_call(CreateFileMapping);
	}
	return hResult;
}

HANDLE unij_open_mmap(const wchar_t* name, bool readonly)
{
	HANDLE result = NULL;
	DWORD access = readonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS; 
	ASSERT_VALID_STRING(name);
	result = OpenFileMappingW(access, FALSE, name);
	if(IS_INVALID_HANDLE(result)) {
		unij_fatal_error(UNIJ_ERROR_LASTERROR, L"Failed call to OpenFileMapping(%lu, FALSE, \"%s\")", access, name);
	}
	return result;
}

HANDLE unij_create_event(const wchar_t* name)
{
	HANDLE result = NULL;
	ASSERT_VALID_STRING(name);
	result = CreateEventW(NULL, TRUE, FALSE, name);

	if(IS_INVALID_HANDLE(result)) {
		unij_fatal_call(CreateEventW);
	}
	return result;
}
