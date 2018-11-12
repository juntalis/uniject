/**
 * @file win32_api.c
 * TODO: description
 */
#include "pch.h"
#include <uniject/win32.h>
#include <uniject/utility.h>
#include <uniject/error.h>

#include <intrin.h>
#pragma intrinsic(_InterlockedCompareExchangePointer)

// Privileges
#define LUID_SIZE(COUNT)  ((uint32_t)(sizeof(LUID_AND_ATTRIBUTES) * (COUNT)))
#define PRIVILEGES_OFFSET ((uint32_t)FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges))

// Private run once primitive declaration
struct unij_once_internal
{
	UNIJ_CACHE_ALIGN
	HANDLE event_handle;
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
	
	hExisting = _InterlockedCompareExchangePointer(&pOnce->event_handle, hCreated, NULL);
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

bool unij_once(unij_once_t* pOnce, unij_once_fn fnOnce, void* pData)
{
	BOOL bResult;
	/* Fast case - avoid WaitForSingleObject. */
	unij_once_internal_t* pOncePriv = (unij_once_internal_t*)pOnce;
	bResult = pOncePriv->executed ? pOncePriv->result : unij_once_inner(pOncePriv, fnOnce, pData);
	return bResult ? true : false;
}

static UNIJ_INLINE
PTOKEN_PRIVILEGES unij_alloc_token_privileges(const wchar_t** pNames, uint32_t uCount, LPDWORD lpdwBufferSize)
{
	PTOKEN_PRIVILEGES lpPrivileges = NULL;
	*lpdwBufferSize = PRIVILEGES_OFFSET + LUID_SIZE(uCount);

	lpPrivileges = (PTOKEN_PRIVILEGES)unij_alloc((size_t) *lpdwBufferSize);
	if(lpPrivileges != NULL) {
		uint32_t idx = 0;
		lpPrivileges->PrivilegeCount = uCount;
		for(; idx < uCount; idx++) {
			if(!LookupPrivilegeValueW(NULL, pNames[idx], &lpPrivileges->Privileges[idx].Luid)) {
				unij_free((void*)lpPrivileges);
				lpPrivileges = NULL;
				break;
			}

			lpPrivileges->Privileges[idx].Attributes = SE_PRIVILEGE_ENABLED;
		}
	}

	return lpPrivileges;
}

bool unij_acquire_privileges(HANDLE hProcess, const wchar_t** pNames, uint32_t uCount)
{
	HANDLE hToken = NULL;
	BOOL bResult = FALSE;

	if(OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken )) {
		DWORD dwBufferSize = 0, dwLastError = ERROR_SUCCESS, i = 0;
		PTOKEN_PRIVILEGES lpPrivileges = unij_alloc_token_privileges(pNames, uCount, &dwBufferSize);

		// Mark the call as successful
		if(lpPrivileges != NULL) {
			bResult = AdjustTokenPrivileges(hToken, FALSE, lpPrivileges, dwBufferSize, NULL, NULL);
			if(bResult == FALSE)
				dwLastError = GetLastError();
			unij_free((void*)lpPrivileges);
		} else {
			dwLastError = GetLastError();
		}
		
		CloseHandle(hToken);
		SetLastError(dwLastError);
	}
	
	return bResult ? true : false;
}

static UNIJ_NOINLINE
BOOL CDECL unij_acquire_default_privileges_once(void* pData)
{
	static const wchar_t* pNames[] = UNIJ_SE_TOKENS;
	static uint32_t uNamesCount = (uint32_t)ARRAYLEN(pNames);
	return unij_acquire_privileges(GetCurrentProcess(), pNames, uNamesCount);
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

const wchar_t* unij_object_name(const wchar_t* key, unij_object_t type, uint32_t pid)
{
	const wchar_t* result = NULL;
	const wchar_t* stype = unij_object_type_text(type);
	ASSERT_NOT_ZERO(pid);
	ASSERT_VALID_STRING(key);
	result = unij_sawprintf(UNIJ_OBJECT_FORMAT, key, stype, pid);
	ASSERT_VALID_STRING(result);
	return result;
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
	HANDLE hResult = NULL;
	DWORD dwAccess = readonly ? PAGE_READONLY : PAGE_READWRITE; 
	ASSERT_VALID_STRING(name);
	hResult = OpenFileMappingW(dwAccess, FALSE, name);
	if(IS_INVALID_HANDLE(hResult)) {
		unij_fatal_call(OpenFileMapping);
	}
	return hResult;
}

HANDLE unij_create_event(const wchar_t* name)
{
	HANDLE hResult = NULL;
	ASSERT_VALID_STRING(name);
	hResult = CreateEventW(NULL, TRUE, FALSE, name);

	if(IS_INVALID_HANDLE(hResult)) {
		unij_fatal_call(CreateEventW);
	}
	return hResult;
}
