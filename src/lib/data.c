/**
 * @file data.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include <uniject/data.h>
#include <uniject/error.h>
#include <uniject/utility.h>
#include <uniject/ipc.h>

#define PACKER_MODE_RESERVE 0
#define PACKER_MODE_PACK    1

struct unij_packer
{
	uint8_t mode;
	size_t size;
	void* position;
	const void* buffer;
	unij_memprocs_t* mem;
};

struct unij_unpacker
{
	size_t size;
	void* position;
	const void* buffer;
};

/**
 * @internal
 * Internally used helpers
 */

#define ENSURE_PACKER(P) \
	( unij_ensure_context(P, L"packer", __FUNCTIONW__) )

#define ENSURE_UNPACKER(U) \
	( unij_ensure_context(U, L"unpacker", __FUNCTIONW__) )

#define ENSURE_PACK_MODE(P) \
	( unij_packer_ensure_mode(P, PACKER_MODE_PACK, __FUNCTIONW__) )

#define ENSURE_RESERVE_MODE(P) \
	( unij_packer_ensure_mode(P, PACKER_MODE_RESERVE, __FUNCTIONW__) )

#define TOSZ(X) ((size_t)(X))

static const wchar_t WHEN_BEFORE[] = L"before";
static const wchar_t WHEN_AFTER[]  = L"after";

static const wchar_t BOUNDS_UPPER[] = L"upper";
static const wchar_t BOUNDS_LOWER[] = L"lower";

static bool unij_ensure_context(void* ctx, const wchar_t* which, const wchar_t* funcName)
{
	bool status = true;
	if(ctx == NULL) {
		status = false;
		unij_fatal_error(
			UNIJ_ERROR_ADDRESS,
			L"Fatal call made to %s with NULL %s",
			funcName, which
		);
 	}
 	return status;
}

static bool unij_packer_ensure_mode(unij_packer_t* P, uint8_t mode, const wchar_t* funcName)
{
	bool status = true;
	if(P->mode != mode) {
		const wchar_t* when = mode == PACKER_MODE_RESERVE ? WHEN_BEFORE : WHEN_AFTER;
		status = false;
		unij_fatal_error(
			UNIJ_ERROR_OPERATION,
			L"Calls to %s can only be made %s the operation mode is changed with a call to unij_packer_commit!",
			funcName, when
		);
	}
	return status;
}

static void unij_packer_reset_internal(unij_packer_t* P)
{
	// If the packer has already transitioned to packing mode, wee need to do some additional cleanup.
	if(P->mode == PACKER_MODE_PACK && P->buffer != NULL) {
		unij_memprocs_t* memprocs = P->mem;
		// Free packer buffer
		ASSERT_NOT_NULL(memprocs->free_fn);
		memprocs->free_fn(memprocs->parameter, (void*)P->buffer);
	}
	
	// Revert the fields back to the starting state
	RtlZeroMemory((void*)P, sizeof(*P));
}

/**
 * @brief Sets the packer's initial state
 * @param P packer
 * @param memprocs Memory routines 
 */
static UNIJ_INLINE void unij_packer_initial(unij_packer_t* P, unij_memprocs_t* memprocs)
{
	P->mem = memprocs;
	P->size = sizeof(size_t);
	P->mode = PACKER_MODE_RESERVE; // To make it more explicit
}

static UNIJ_INLINE size_t unij_packer_bytes_used(unij_packer_t* P)
{
	return TOSZ(P->position) - TOSZ(P->buffer);
}

static UNIJ_INLINE size_t unij_packer_bytes_remaining(unij_packer_t* P)
{
	size_t szUsed = unij_packer_bytes_used(P);
	assert(szUsed <= P->size);
	return P->size - szUsed;
}

static UNIJ_INLINE size_t unij_unpacker_bytes_used(unij_unpacker_t* U)
{
	return TOSZ(U->position) - TOSZ(U->buffer);
}

static UNIJ_INLINE size_t unij_unpacker_bytes_remaining(unij_unpacker_t* U)
{
	size_t szUsed = unij_unpacker_bytes_used(U);
	assert(szUsed <= U->size);
	return U->size - szUsed;
}

/**
 * @endinternal
 */

unij_packer_t* unij_packer_create(unij_memprocs_t* memprocs)
{
	unij_packer_t* P;
	
	// Should never be NULL
	ASSERT_NOT_NULL(memprocs);
	ASSERT_NOT_NULL(memprocs->alloc_fn);
	ASSERT_NOT_NULL(memprocs->free_fn);
	
	// Allocate
	P = (unij_packer_t*)unij_alloc(sizeof(*P));
	if(P != NULL) {
		unij_packer_initial(P, memprocs);
	} else {
		unij_fatal_alloc();
	}
	return P;
}

void unij_packer_reset(unij_packer_t* P)
{
	// Only the memory routines are preserved. 
	unij_memprocs_t* memprocs;
	if(!ENSURE_PACKER(P)) return;
	memprocs = P->mem;
	
	// Use internal reset handler to zero everything out.
	unij_packer_reset_internal(P);
	
	// Then revert the fields to the initial state at the time of construction.
	unij_packer_initial(P, memprocs);
}

void unij_packer_destroy(unij_packer_t* P)
{
	if(P != NULL) {
		unij_packer_reset_internal(P);
		unij_free((void*)P);
	}
}

size_t unij_packer_size(unij_packer_t* P)
{
	return ENSURE_PACKER(P) ? P->size : 0;
}

void unij_reserve(unij_packer_t* P, size_t size)
{
	if(ENSURE_PACKER(P) && ENSURE_RESERVE_MODE(P)) {
		P->size += size;
	}
}

void unij_reserve_wstr(unij_packer_t* P, unij_wstr_t data)
{
	// Mode/NULL checks are done in unij_reserve call
	size_t szBytes = sizeof(data.length) + (data.length * sizeof(wchar_t));
	unij_reserve(P, szBytes);
}

bool unij_packer_commit(unij_packer_t* P)
{
	bool status = true;
	void*  buffer = NULL;
	unij_memprocs_t* memprocs;
	if(!ENSURE_PACKER(P) || !ENSURE_RESERVE_MODE(P))
		return false;
	
	// Create a local reference
	memprocs = P->mem;
	buffer = memprocs->alloc_fn(memprocs->parameter, P->size);
	if(buffer == NULL) {
		status = false;
		unij_fatal_error(
			UNIJ_ERROR_OUTOFMEMORY,
			L"Specified unij_memprocs_t::alloc_fn failed to alloc packer buffer of %zu bytes",
			P->size
		);
	} else {
		RtlZeroMemory((void*)buffer, P->size);
		P->position = buffer;
		P->buffer = (const void*)buffer;
		P->mode = PACKER_MODE_PACK;
		status = unij_pack(P, (const void*)&(P->size), sizeof(size_t));
	}
	return status;
}

const void* unij_packer_buffer(unij_packer_t* P)
{
	const void* result = NULL;
	if(ENSURE_PACKER(P) && ENSURE_PACK_MODE(P)) {
		result = P->buffer;
	}
	return result;
}

bool unij_pack(unij_packer_t* P, const void* data, size_t size)
{
	bool status = true;
	size_t szRemaining;
	
	if(!ENSURE_PACKER(P) || !ENSURE_PACK_MODE(P))
		return false;
	
	// Empty data is silently ignored.
	if(data == NULL || size == 0) 
		return true;
	
	szRemaining = unij_packer_bytes_remaining(P);
	if(size > szRemaining) {
		status = false;
		unij_fatal_error(
			UNIJ_ERROR_OPERATION,
			L"Attempted to pack %zu bytes with only %zu bytes of space left in the packer's buffer. "
			L"Verify that all data packing calls has a matching reserve.",
			size, szRemaining
		);
	} else {
		void* pDest = P->position;
		RtlCopyMemory(pDest, (const void*)data, size);
		P->position = (void*)(TOSZ(P->position) + size);
	}
	
	return status;
}

bool unij_pack_wstr(unij_packer_t* P, unij_wstr_t data)
{
	return unij_pack_val(P, data.length) &&
	       unij_pack(P, (const void*)data.value, data.length * sizeof(wchar_t));
}

unij_unpacker_t* unij_unpacker_create(const void* buffer)
{
	unij_unpacker_t* U;
	
	// Should never be NULL
	if(buffer == NULL) {
		unij_fatal_error(
			UNIJ_ERROR_ADDRESS,
			L"Fatal call made to unij_unpacker_create with a NULL buffer!"
		);
		return NULL;
	}
	
	// Allocate unpacker struct
	U = (unij_unpacker_t*)unij_alloc(sizeof(*U));
	if(U != NULL) {
		U->buffer = (const void*)buffer;
		U->size = *((size_t*)buffer);
		U->position = (void*)(TOSZ(buffer) + sizeof(size_t));
	} else {
		unij_fatal_alloc();
	}
	return U;
}

void unij_unpacker_destroy(unij_unpacker_t* U)
{
	if(U != NULL) {
		RtlZeroMemory((void*)U, sizeof(*U));
		unij_free((void*)U);
	}
}

void* unij_unpacker_peek(unij_unpacker_t* U)
{
	return ENSURE_UNPACKER(U) ? U->position : NULL;
}

bool unij_unpacker_seek(unij_unpacker_t* U, int64_t offset)
{
	bool result = false;
	if(ENSURE_UNPACKER(U)) {
		size_t szOffset, szBoundOffset;
		const wchar_t* bounds_text;
		
		// Determine which direction we'll be bounds checking against. Negative offset => lower bound,
		// positive offset or zero => upper bound
		bool lower_bound = offset < 0;
		if(lower_bound) {
			// COmpare offset against the number of bytes we've already unpacked.
			szBoundOffset = unij_unpacker_bytes_used(U);
			
			// Flip it to a positive number so we can compare unsigned values
			bounds_text = BOUNDS_LOWER;
			szOffset = (size_t)((int64_t)(-1 * offset));
			result = szOffset <= szBoundOffset;
			
			// Subtract the offset from our position
			if(result) U->position = (void*)(TOSZ(U->position) - szOffset);
			
		} else {
			// Compare offset against the remaining bytes in our buffer
			szBoundOffset = unij_unpacker_bytes_remaining(U);
			szOffset = (size_t)offset;
			bounds_text = BOUNDS_UPPER;
			result = szOffset <= szBoundOffset;
			
			// Add the offset to our position
			if(result) U->position = (void*)(TOSZ(U->position) + szOffset);
		}
		
		// On failure, throw an error.
		if(!result) {
			unij_fatal_error(
				UNIJ_ERROR_OPERATION,
				L"Attempted to reposition the unpacker outside of the buffer's %s bounds. Seek "
				L"offset %zu > bounds offset %zu",
				bounds_text, szOffset, szBoundOffset
			);
		}
	}
	return result;
}

bool unij_unpack(unij_unpacker_t* U, void* pDest, size_t size)
{
	bool status = true;
	size_t szRemaining;
	
	// Verify packer
	if(!ENSURE_UNPACKER(U))
		return false;
	
	// Verify our destination
	if(pDest == NULL) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"Fatal call made to unij_unpack: NULL destination");
		return FALSE;
	}
	
	szRemaining = unij_unpacker_bytes_remaining(U);
	if(size > szRemaining) {
		status = false;
		unij_fatal_error(
			UNIJ_ERROR_OPERATION,
			L"Attempted to unpack %zu bytes with only %zu bytes left in the buffer. Verify the data layout for  "
			L"your packer and unpacker match up. (if so, please report this issue)",
			size, szRemaining
		);
	} else {
		RtlCopyMemory(pDest, (const void*)U->position, size);
		U->position = (void*)(TOSZ(U->position) + size);
	}
	
	return status;
}

bool unij_unpack_wstr(unij_unpacker_t* U, unij_wstr_t* pDest)
{
	bool success = false;
	
	// Verify our destination
	if(pDest == NULL) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"Fatal call made to unij_unpack_wstr: NULL destination");
		return FALSE;
	}
	
	// Zero out the destination. unpacker NULL checking done by unij_unpack.
	RtlZeroMemory((void*)pDest, sizeof(*pDest));
	if(unij_unpack_val(U, &pDest->length) && (pDest->length > 0)) {
		size_t byteCount = WSIZE(pDest->length);
		pDest->value = (const wchar_t*)unij_unpacker_peek(U);
		success = unij_unpacker_seek(U, byteCount);
		if(!success) {
			unij_show_error_message(
				L"Failed to seek to the end of a packed wstr. Verify that the data layout for your packer "
				L"and unpacker match. (if so, please report this issue)"
			);
		}
	}
	return success;
}

