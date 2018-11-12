/**
 * @file ipc.c
 * @author Charles Grunwald <cgrunwald@bluewaterads.com>
 */
#include "pch.h"
#include <uniject/ipc.h>
#include <uniject/data.h>
#include <uniject/error.h>
#include <uniject/win32.h>
#include <uniject/params.h>
#include <uniject/utility.h>

struct unij_ipc
{
	uint32_t pid;
	unij_role_t role;
	const wchar_t* name;
	HANDLE file_handle;
	void* mapped_view;
	HANDLE event_handle;
	unij_reserve_fn reserve_fn;
	union
	{
		unij_pack_fn pack_fn;
		unij_unpack_fn unpack_fn;
	};
	unij_memprocs_t memprocs;
};

static UNIJ_INLINE bool unij_ipc_ensure_ctx(unij_ipc_t* ctx, const wchar_t* caller)
{
	bool status = true;
	if(ctx == NULL) {
		status = false;
		unij_fatal_error(
			UNIJ_ERROR_ADDRESS,
			L"Fatal call made to %s with NULL unij_ipc_t*!", caller
		);
 	}
 	return status;
}

#define ENSURE_CTX(CTX) \
	( unij_ipc_ensure_ctx(CTX, __FUNCTIONW__) )

static UNIJ_INLINE const wchar_t* unij_ipc_role_text(unij_role_t role)
{
	switch(role)
	{
		case UNIJ_ROLE_INJECTOR:
			return L"injector";
		case UNIJ_ROLE_LOADER:
			return L"loader";
		default:
			return L"unknown";
	}
}

static bool unij_ipc_ensure_role(unij_ipc_t* ctx, unij_role_t role, const wchar_t* caller)
{
	bool status = unij_ipc_ensure_ctx(ctx, caller);
	if(status) {
		if(ctx->role != role) {
			const wchar_t* expected_role = unij_ipc_role_text(role); 
			status = false;
			unij_fatal_error(
				UNIJ_ERROR_OPERATION,
				L"Calls to %s can only be made from an IPC context with the role: %s!",
				caller, expected_role
			);
		}
	}
	return status;
}

#define ENSURE_INJECTOR(CTX) \
	( unij_ipc_ensure_role(CTX, UNIJ_ROLE_INJECTOR, __FUNCTIONW__) )

#define ENSURE_LOADER(CTX) \
	( unij_ipc_ensure_role(CTX, UNIJ_ROLE_LOADER, __FUNCTIONW__) )

static void* unij_ipc_open_mapped_view(unij_ipc_t* ctx)
{
	// Set mapped view
	if(ctx->mapped_view == NULL) {
		ctx->mapped_view = MapViewOfFile(ctx->file_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	}
	return ctx->mapped_view;
}

static void* unij_ipc_loader_open_mapping(unij_ipc_t* ctx)
{
	void* result = NULL;
	if(!ENSURE_LOADER(ctx)) {
		return NULL;
	}
	
	// Set file_handle
	if(IS_INVALID_HANDLE(ctx->file_handle)) {
		ctx->file_handle = unij_open_mmap(ctx->name, true);
		if(IS_INVALID_HANDLE(ctx->file_handle))
			return NULL;
	}
	
	result = unij_ipc_open_mapped_view(ctx);
	if(result == NULL) {
		CloseHandle(ctx->file_handle);
		ctx->file_handle = NULL;
	}
	
	return result;
}

// These two are only actually used by the injector.
static UNIJ_NOINLINE void* CDECL unij_ipc_injector_alloc(unij_ipc_t* ctx, size_t size)
{
	void* result = NULL;
	ASSERT_NOT_ZERO(size);
	if(!ENSURE_INJECTOR(ctx)) {
		return NULL;
	}
	
	// Set file_handle
	if(IS_INVALID_HANDLE(ctx->file_handle)) {
		ctx->file_handle = unij_create_mmap(ctx->name, size);
		if(IS_INVALID_HANDLE(ctx->file_handle))
			return NULL;
	}
	
	result = unij_ipc_open_mapped_view(ctx);
	if(result == NULL) {
		CloseHandle(ctx->file_handle);
		ctx->file_handle = NULL;
	}
	
	return result;
}

static UNIJ_NOINLINE void CDECL unij_ipc_injector_free(unij_ipc_t* ctx, void* ptr)
{
	if(ptr == NULL || !ENSURE_INJECTOR(ctx)) return;
	
	UnmapViewOfFile((LPCVOID)ptr);
	if(ctx->mapped_view != ptr && ctx->mapped_view != NULL) {
		UnmapViewOfFile((LPCVOID)ctx->mapped_view);
	}
	ctx->mapped_view = NULL;
}

static unij_ipc_t* unij_ipc_common_create(uint32_t pid, const wchar_t* key)
{
	DWORD dwError = ERROR_SUCCESS;
	const wchar_t* event_name = NULL;
	unij_ipc_t* ctx = (unij_ipc_t*)unij_alloc(sizeof(*ctx));
	if(ctx == NULL) {
		unij_fatal_alloc();
		return NULL;
	}
	
	ctx->memprocs.parameter = (void*)ctx;
	ctx->memprocs.alloc_fn = (unij_alloc_fn)unij_ipc_injector_alloc;
	ctx->memprocs.free_fn = (unij_free_fn)unij_ipc_injector_free;
	
	if(pid == 0) {
		ctx->pid = (uint32_t)GetCurrentProcessId();
		ctx->role = UNIJ_ROLE_LOADER;
		ctx->unpack_fn = (unij_unpack_fn)unij_unpack_params;
	} else {
		ctx->pid = pid;
		ctx->role = UNIJ_ROLE_INJECTOR;
		ctx->pack_fn = (unij_pack_fn)unij_pack_params;
		ctx->reserve_fn = (unij_reserve_fn)unij_reserve_params;
	}
	
	ctx->name = unij_object_name(key, UNIJ_OBJECT_MAPPING, ctx->pid);
	if(ctx->name == NULL) {
		unij_ipc_destroy(ctx);
		return NULL;
	}
	
	event_name = unij_object_name(key, UNIJ_OBJECT_EVENT, ctx->pid);
	ctx->event_handle = unij_create_event(event_name);
	dwError = GetLastError();
	unij_free((void*)event_name);
	if(IS_INVALID_HANDLE(ctx->event_handle)) {
		unij_ipc_destroy(ctx);
		ctx = NULL;
	} else if(ctx->role == UNIJ_ROLE_INJECTOR && dwError == ERROR_ALREADY_EXISTS) {
		SetLastError(dwError);
		unij_fatal_call(CreateEvent);
		unij_ipc_destroy(ctx);
		ctx = NULL;
	} else if(dwError == ERROR_ALREADY_EXISTS) {
		SetLastError(ERROR_SUCCESS);
	}
	
	return ctx;
}

unij_ipc_t* unij_ipc_injector_create(uint32_t pid, const wchar_t* key)
{
	return unij_ipc_common_create(pid, key);
}

unij_ipc_t* unij_ipc_loader_create(const wchar_t* key)
{
	return unij_ipc_common_create(0, key);
}

bool unij_ipc_injector_wait(unij_ipc_t* ctx)
{
	bool result = ENSURE_INJECTOR(ctx);
	
	if(result && IS_VALID_HANDLE(ctx->event_handle)) {
		HANDLE hEvent = ctx->event_handle;
		ctx->event_handle = NULL;
		result = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		CloseHandle(hEvent);
	}
	return result;
}

bool unij_ipc_loader_notify(unij_ipc_t* ctx)
{
	bool result = ENSURE_LOADER(ctx);
	if(result && IS_VALID_HANDLE(ctx->event_handle)) {
		HANDLE hEvent = ctx->event_handle;
		ctx->event_handle = NULL;
		result = SetEvent(hEvent) ? true : false;
		CloseHandle(hEvent);
	}
	return result;
}


static void unij_ipc_close(unij_ipc_t* ctx)
{
	if(ctx != NULL) {
		if(ctx->mapped_view != NULL) {
			UnmapViewOfFile((LPCVOID)ctx->mapped_view);
			ctx->mapped_view = NULL;
		}
	
		if(IS_VALID_HANDLE(ctx->file_handle)) {
			CloseHandle(ctx->file_handle);
			ctx->file_handle = NULL;
		}
	}
}

void unij_ipc_destroy(unij_ipc_t* ctx)
{
	if(ctx != NULL) {
		if(ctx->name != NULL) {
			unij_free((void*)ctx->name);
			ctx->name = NULL;
		}
		
		unij_ipc_close(ctx);
		
		if(IS_VALID_HANDLE(ctx->event_handle)) {
			CloseHandle(ctx->event_handle);
			ctx->event_handle = NULL;
		}
		
		unij_free(ctx);
	}
}

void unij_ipc_set_reserve_fn(unij_ipc_t* ctx, unij_reserve_fn fn)
{
	if(ENSURE_CTX(ctx)) {
		ctx->reserve_fn = fn;
	}
}

void unij_ipc_set_pack_fn(unij_ipc_t* ctx, unij_pack_fn fn)
{
	if(ENSURE_INJECTOR(ctx)) {
		ctx->pack_fn = fn;
	}
}

void unij_ipc_set_unpack_fn(unij_ipc_t* ctx, unij_unpack_fn fn)
{
	if(ENSURE_LOADER(ctx)) {
		ctx->unpack_fn = fn;
	}
}



bool unij_ipc_pack(unij_ipc_t* ctx, const void* data)
{
	unij_packer_t* P;
	if(!ENSURE_INJECTOR(ctx)) return false;
	if(data == NULL) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"NULL data passed into unij_ipc_pack!");
		return false;
	}
	
	// Create packer
	P = unij_packer_create(&ctx->memprocs);
	if(P == NULL) {
		unij_ipc_close(ctx);
		return false;
	}
	
	ctx->reserve_fn(P, data);
	if(!unij_packer_commit(P) || !ctx->pack_fn(P, data)) {
		unij_fatal_error(UNIJ_ERROR_OPERATION, L"Failed while packing IPC data");
		unij_packer_destroy(P);
		unij_ipc_close(ctx);
		return false;
	} else {
		// Log("SUCCESS");
	}
	
	unij_packer_destroy(P);
	return true;
}

bool unij_ipc_unpack(unij_ipc_t* ctx, void* dest)
{
	const void* buffer;
	unij_unpacker_t* U;
	if(!ENSURE_LOADER(ctx)) return false;
	if(dest == NULL) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"NULL dest passed into unij_ipc_unpack!");
		return false;
	}
	
	// Open file handle and map view of file.
	buffer = unij_ipc_loader_open_mapping(ctx);
	if(buffer == NULL) {
		return false;
	}
	
	// Create our unpacker
	U = unij_unpacker_create(buffer);
	if(U == NULL) {
		unij_ipc_close(ctx);
		return false;
	}
	
	// Apply our unpack handler
	if(!ctx->unpack_fn(U, dest)) {
		unij_fatal_error(UNIJ_ERROR_OPERATION, L"Failed while unpacking IPC data");
		unij_unpacker_destroy(U);
		unij_ipc_close(ctx);
		return false;
	} else {
		// Log("SUCCESS");
	}
	
	unij_unpacker_destroy(U);
	return true;
}


#if 0

static unij_ipc* CreateParamsCtx(const wchar_t* pName, HANDLE hMapping, uint32_t dwRole)
{
	unij_ipc* pResult;
	uint32_t dwAccess = dwRole = ROLE_INJECTOR ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
	pResult = calloc(1, sizeof(*pResult));
	ASSERT_NOT_NULL(pResult);
	pResult->Name = pName;
	pResult->Role = dwRole;
	pResult->Mapping = hMapping;
	pResult->View = MapViewOfFile(pResult->Mapping, dwAccess, 0, 0, 0);
	if(pResult->View == NULL) {
		unij_free((void*)pResult);
		pResult = NULL;
		unij_fatal_call(MapViewOfFile);
	}
	return pResult;
}

unij_ipc* InitializeParams(unij_params_t* pSrc)
{
	size_t szParams;
	const wchar_t* pName = NULL;
	HANDLE hMapping = NULL;
	bool bFreeLogfile = FALSE;
	unij_ipc* pResult = NULL;
	unij_params_packed oStaging = {0};
	
	// Verify required fields and fill in the defaults for unspecified optional fields.
	if(!ProcessUnpackedParams(pSrc)) {
		LogError("pParams failed validation! Returning NULL..");
		return NULL;
	}
	
	// Calculate the necessary size for our shared memory.
	szParams = GetPackedParamsSize(&oStaging, pSrc);
	if(oStaging.LogfileLength == 0) {
		bFreeLogfile = TRUE;
		pSrc->LogfilePath = GenerateLogfilePath(&oStaging, pSrc);
		szParams += oStaging.LogfileLength * sizeof(wchar_t);
	}
	
	// Create our file mapping.
	hMapping = CreateSharedMapping(&pName, UNIJ_PARAMS_KEYW, pSrc->PID, szParams);
	if(hMapping == NULL) {
		goto cleanup;
	}
	
	// Success - let's build out parameter.
	pResult = CreateParamsCtx(pName, hMapping, ROLE_INJECTOR);
	if(pResult == NULL) {
		goto cleanup;
	}
	
	// Pack our params
	PackParamsView(pResult->View, &oStaging, pSrc);
	
cleanup:
	// Cleanup LogPath if necessary.
	if(bFreeLogfile) {
		free((void*)pSrc->LogfilePath);
		pSrc->LogfilePath = NULL;
	}
	
	// Cleanup pName if necessary
	if(pResult ==  NULL && pName != NULL) {
		unij_free((void*)pName);
	}
	
	return pResult;
}
#endif
