/**
 * @file ipc.c
 * @author Charles Grunwald <ch@rles.rocks>
 * 
 * NOTE: \a unij_ipc declared in \a base_private.h
 */
#include "pch.h"
#include "base_private.h"

#include <uniject/ipc.h>
#include <uniject/logger.h>
#include <uniject/packing.h>
#include <uniject/params.h>
#include <uniject/process.h>
#include <uniject/utility.h>
#include <uniject/win32.h>

#define IPC_MAX_SIZE 0x1000

#define ROLE_READER ROLE_LOADER
#define ROLE_WRITER ROLE_INJECTOR

#define AS_UPTR(X) ((uintptr_t)(X))

#define ENSURE_WRITER(IPC) \
	( ipc_ensure_role(IPC, ROLE_WRITER, __FUNCTIONW__) )

#define ENSURE_READER(IPC) \
	( ipc_ensure_role(IPC, ROLE_READER, __FUNCTIONW__) )

static UNIJ_INLINE unij_role_t ipc_get_role(unij_ipc_t* ipc)
{
	return ipc->custom ? (unij_role_t)AS_UPTR(ipc->role) : *ipc->role; 
}

static UNIJ_INLINE uniject_t* ipc_get_ctx(unij_ipc_t* ipc)
{
	return ipc->custom ? NULL : (uniject_t*)(AS_UPTR(ipc->role) - FIELD_OFFSET(uniject_t, role));
}

static UNIJ_INLINE const wchar_t* ipc_role_name(unij_role_t role)
{
	switch(role)
	{
		case ROLE_WRITER:
			return L"writer";
		case ROLE_READER:
			return L"reader";
		default:
			return L"unknown";
	}
}

static bool ipc_ensure_role(unij_ipc_t* ipc, unij_role_t expected, const wchar_t* caller)
{
	bool status = !unij_fatal_null2(ipc, caller);
	if(status) {
		unij_role_t role = ipc_get_role(ipc);
		if(role != expected) {
			const wchar_t* expected_role = ipc_role_name(expected); 
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

static UNIJ_INLINE void ipc_close_handle(unij_ipc_t* ipc)
{
	// We are ensured of a non-NULL ipc by all callers to this function.
	if(IS_VALID_HANDLE(ipc->file_handle)) {
		CloseHandle(ipc->file_handle);
		ipc->file_handle = NULL;
	}
}

static void ipc_close_mmap(unij_ipc_t* ipc, bool close_handle)
{
	// We are ensured of a non-NULL ipc by all callers to this function.
	if(ipc->mapped_view != NULL) {
		UnmapViewOfFile((const void*)ipc->mapped_view);
		ipc->mapped_view = NULL;
	}
	
	if(close_handle)
		ipc_close_handle(ipc);
}

static UNIJ_INLINE HANDLE ipc_ensure_handle(unij_ipc_t* ipc, size_t size, bool readonly)
{
	if(IS_INVALID_HANDLE(ipc->file_handle))
		ipc->file_handle = size == 0 ? unij_open_mmap(ipc->name, readonly) : unij_create_mmap(ipc->name, size);
	return ipc->file_handle;
}

static UNIJ_INLINE void* ipc_create_reader_mmap(unij_ipc_t* ipc, uint32_t access)
{
	size_t size;
	size_t* pmapsize = (size_t*)MapViewOfFile(ipc->file_handle, access, 0, 0, sizeof(size_t));
	if(pmapsize == NULL) {
		return NULL;
	}
	size = *pmapsize;
	UnmapViewOfFile((const void*)pmapsize);
	return MapViewOfFile(ipc->file_handle, access, 0, 0, size);
}

static void* ipc_ensure_mmap(unij_ipc_t* ipc, size_t size)
{
	// Ensure we have a file handle to work with.
	bool readonly = UNIJ_LOADER_READONLY;
	HANDLE file_handle = ipc_ensure_handle(ipc, size, readonly);
	if(IS_INVALID_HANDLE(file_handle))
		return NULL;
	
	// Set mapped view
	if(ipc->mapped_view == NULL) {
		uint32_t access = readonly && size == 0 ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
		ipc->mapped_view = MapViewOfFile(ipc->file_handle, access, 0, 0, 0);
		if(ipc->mapped_view == NULL) {
			DWORD last_error = GetLastError();
			ipc_close_handle(ipc);
			SetLastError(last_error);
			unij_fatal_call(MapViewOfFile);
		}
	}
	
	return ipc->mapped_view;
}

// Used internally for the writer context.
static UNIJ_NOINLINE void* CDECL ipc_packer_alloc_handler(unij_ipc_t* ipc, size_t size)
{
	if(!ENSURE_WRITER(ipc)) {
		return NULL;
	} else if(size == 0) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Somehow ipc_packer_alloc_handler got called "
		                                      L"with size: 0. This should not happen.");
		return NULL;
	}
	
	UNIJ_SHOW_INFO(L"ipc_packer_alloc_handler hit: size=%zu", size);
	return ipc_ensure_mmap(ipc, size);
}

// Used internally for the writer context.
//
// IMPORTANT: We are intentionally not closing the file handle. This is to make sure the shared memory doesn't get
// cleaned up before the loader has a chance to read it. DO NOT FORGET.
static UNIJ_NOINLINE void CDECL ipc_packer_free_handler(unij_ipc_t* ipc, void* ptr)
{
	if(ptr == NULL || !ENSURE_WRITER(ipc)) return;
	if(ipc->mapped_view != ptr && ipc->mapped_view != NULL) {
		LogWarning(L"ipc_packer_free_handler called with a pointer that didn't match ipc->mapped_view. Should not happen.");
		ipc_close_mmap(ipc, false);
	}
	UnmapViewOfFile((const void*)ptr);
}

static bool ipc_common_open(unij_ipc_t* ipc, uint32_t pid, const wchar_t* key)
{
	unij_role_t role = ipc_get_role(ipc);
	
	if(role == ROLE_READER) {
		ipc->unpack_fn = (unij_unpack_fn)unij_unpack_params;
	} else {
		ipc->pack_fn = (unij_pack_fn)unij_pack_params;
		ipc->reserve_fn = (unij_reserve_fn)unij_reserve_params;
		ipc->memprocs.parameter = (void*)ipc;
		ipc->memprocs.alloc_fn = (unij_alloc_fn)ipc_packer_alloc_handler;
		ipc->memprocs.free_fn = (unij_free_fn)ipc_packer_free_handler;
	}
	
	ipc->name = unij_object_name(key, UNIJ_OBJECT_MAPPING, pid);
	if(ipc->name == NULL) {
		unij_fatal_alloc();
		return false;
	}
	
	return true;
}

static unij_ipc_t* ipc_custom_open(uint32_t pid, const wchar_t* key, unij_role_t role)
{
	unij_ipc_t* ipc = (unij_ipc_t*)unij_alloc(sizeof(*ipc));
	if(ipc == NULL) {
		unij_fatal_alloc();
		return NULL;
	}
	
	// No use wasting an allocation when a pointer can fit any role value.
	ipc->custom = true;
	ipc->role = (unij_role_t*)AS_UPTR(role);
	if(!ipc_common_open(ipc, pid, key)) {
		unij_free((void*)ipc);
		ipc = NULL;
	}
	
	return ipc;
}

bool unij_ipc_open(uniject_t* ctx)
{
	unij_ipc_t* ipc = &ctx->ipc;
	RtlZeroMemory((PVOID)ipc, sizeof(*ipc));
	ctx->ipc.custom = false;
	ctx->ipc.role = &ctx->role;
	return ipc_common_open(ipc, unij_get_pid(ctx), UNIJ_PARAMS_KEYW);
}

unij_ipc_t* unij_ipc_writer_open(uint32_t pid, const wchar_t* key)
{
	return ipc_custom_open(pid, key, ROLE_WRITER);
}

unij_ipc_t* unij_ipc_reader_open(const wchar_t* key)
{
	uint32_t pid = (uint32_t)GetCurrentProcessId();
	return ipc_custom_open(pid, key, ROLE_READER);
}

void unij_ipc_close(unij_ipc_t* ipc)
{
	if(ipc != NULL) {
		
		// Free up object name
		if(ipc->name != NULL) {
			unij_free((void*)ipc->name);
			ipc->name = NULL;
		}
		
		// Cleanup mmap if open
		ipc_close_mmap(ipc, true);
		
		// Either custom or standard context.
		if(ipc->custom) {
			unij_free(ipc);
		} else {
			uniject_t* ctx = ipc_get_ctx(ipc);
			RtlZeroMemory((void*)&(ctx->ipc), sizeof(ctx->ipc));
		}
	}
}

void unij_ipc_set_reserve_fn(unij_ipc_t* ipc, unij_reserve_fn fn)
{
	if(ENSURE_WRITER(ipc) && !unij_fatal_null(fn)) {
		ipc->reserve_fn = fn;
	}
}

void unij_ipc_set_pack_fn(unij_ipc_t* ipc, unij_pack_fn fn)
{
	if(ENSURE_WRITER(ipc) && !unij_fatal_null(fn)) {
		ipc->pack_fn = fn;
	}
}

void unij_ipc_set_unpack_fn(unij_ipc_t* ipc, unij_unpack_fn fn)
{
	if(ENSURE_READER(ipc) && !unij_fatal_null(fn)) {
		ipc->unpack_fn = fn;
	}
}

bool unij_ipc_pack(unij_ipc_t* ipc, const void* data)
{
	bool result = false;
	unij_packer_t* P;
	if(!ENSURE_WRITER(ipc) || unij_fatal_null(data))
		return false;
	
	// Create packer
	P = unij_packer_create(&ipc->memprocs);
	if(P == NULL) {
		return result;
	}
	
	ipc->reserve_fn(P, data);
	if(!unij_packer_commit(P) || !ipc->pack_fn(P, data)) {
		// Full closure of mmap, due to failure in our pack or commit functions. 
		unij_packer_destroy(P);
		ipc_close_mmap(ipc, true);
		return false;
	}
	
	// Flush our changes
	FlushViewOfFile(unij_packer_get_buffer(P), unij_packer_get_size(P));
	
	// IMPORTANT: We are intentionally not closing the file handle. This is to make sure the shared memory doesn't get
	// cleaned up before the loader has a chance to read it. DO NOT FORGET.
	ipc_close_mmap(ipc, false);
	unij_packer_destroy(P);
	return true;
}

bool unij_ipc_unpack(unij_ipc_t* ipc, void* dest)
{
	const void* buffer;
	unij_unpacker_t* U;
	if(!ENSURE_READER(ipc) || unij_fatal_null(dest))
		return false;
	
	// Open file handle and map view of file.
	buffer = ipc_ensure_mmap(ipc, 0);
	if(buffer == NULL) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Failed to open file mapping!");
		return false;
	}
	
	// Create our unpacker
	U = unij_unpacker_create(buffer);
	if(U == NULL) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Failed to create unpacker!");
		ipc_close_mmap(ipc, true);
		return false;
	}
	
	// Apply our unpack handler
	if(!ipc->unpack_fn(U, dest)) {
		unij_fatal_error(UNIJ_ERROR_OPERATION, L"Failed while unpacking IPC data");
		unij_unpacker_destroy(U);
		ipc_close_mmap(ipc, true);
		return false;
	}
	
	// IMPORTANT: We are intentionally not cleaning up the mmap/file handle at this point. This is to ensure that the
	// memory pointed to in dest (particularly any wstr_t's) doesn't become invalidated. DON'T FORGET
	//
	// NOTE: the standard params unpacker actually allocates a copy of the shared memory so this doesn't really apply
	// there.
	//ipc_close_mmap(ipc, true);
	unij_unpacker_destroy(U);
	return true;
}
