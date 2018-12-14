/**
 * @file uniject/packing.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Data serialization/deserialization
 * 
 * The approach is super basic and specific to my use case. Both sides (serializer & deserializer) have
 * to know exactly what the data they're working with looks like before going in.
 * 
 * Data is packed into the buffer with no concern for alignment. Numeric types occupy the full size of their
 * type, (ex: 32-bit integer will always be 4 bytes) using the system's native byte order. Strings/bytes are prefixed
 * with a 16-bit unsigned integer containing their lengths. None of the data I'm working with should ever come anywhere
 * close to overflowing that, so that's what we'll go with for now.
 * 
 * Data is unpacked in the same order that it was packed. Numeric types are copied from the buffer. The structs used
 * in unpacking strings/bytes point directly to the string in the buffer, so the calling code should immediately copy
 * the data into its own storage. Unpacked strings will not contain a trailing zero, s
 */
#ifndef _UNIJECT_PACKING_H_
#define _UNIJECT_PACKING_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

#define unij_reserve_type(P,TYPE) \
	(unij_reserve(P, sizeof(TYPE)))

#define unij_pack_val(P,VALUE) \
	(unij_pack(P, (const void*)&(VALUE), sizeof(VALUE)))

#define unij_unpack_val(U,DEST) \
	( unij_unpack( U, (DEST), sizeof(*(DEST)) ) )

// TODO: Make the buffer mechanic a bit more consistent across the two different context types

typedef void*(CDECL* unij_alloc_fn)(void* parameter, size_t size);
typedef void(CDECL* unij_free_fn)(void* parameter, void* ptr);

/**
 * @brief 
 */
struct unij_memprocs
{
	/**
	 * @brief Anything stored in the \a parameter field will be passed into both handlers: \a alloc_fn and \a free_fn.
	 */
	void* parameter;
	
	/**
	 * @brief Should point to an application callback for handling memory allocations.
	 * @param[in] parameter Context parameter - read from the field above.
	 * @param[in] size The requested size of the allocation. (byte count)
	 * @return A newly allocated buffer or NULL in the case of failure.
	 */
	unij_alloc_fn alloc_fn;
	
	/**
	 * @brief Should point to an application callback for handling the cleanup of \a alloc_fn.
	 * @param[in] parameter Context parameter - read from the field above.
	 * @param[in] ptr The allocated memory we're looking to free.
	 * @param[in] size The size of our allocation
	 */
	unij_free_fn free_fn;
};

/**
 * @brief Packer context declaration.
 */
typedef struct unij_packer unij_packer_t;

/**
 * @brief Unpacker context declaration.
 */
typedef struct unij_unpacker unij_unpacker_t;

typedef struct unij_memprocs unij_memprocs_t;

/**
 * @brief 
 * @param[in] procs
 * @return 
 */
unij_packer_t* unij_packer_create(unij_memprocs_t* procs);

/**
 * @brief 
 * @param P 
 */
void unij_packer_reset(unij_packer_t* P);

/**
 * @brief 
 * @param P 
 */
void unij_packer_destroy(unij_packer_t* P);

/**
 * @brief 
 * @param P 
 * @return 
 */
size_t unij_packer_get_size(unij_packer_t* P);

/**
 * @brief 
 * @param P 
 * @return 
 */
const void* unij_packer_get_buffer(unij_packer_t* P);

// "Reservation" mode functions

/**
 * @brief 
 * @param P 
 * @param size 
 */
void unij_reserve(unij_packer_t* P, size_t size);

/**
 * @brief 
 * @param P 
 * @param data 
 */
void unij_reserve_wstr(unij_packer_t* P, const unij_wstr_t* data);

/**
 * @brief 
 * @param P 
 * @return 
 */
bool unij_packer_commit(unij_packer_t* P);

// "Packing Mode" functions

/**
 * @brief 
 * @param P 
 * @param data 
 * @param size 
 * @return 
 */
bool unij_pack(unij_packer_t* P, const void* data, size_t size);

/**
 * @brief 
 * @param P 
 * @param data 
 * @return 
 */
bool unij_pack_wstr(unij_packer_t* P, const unij_wstr_t* data);

/**
 * @brief 
 * @param buffer 
 * @return 
 */
unij_unpacker_t* unij_unpacker_create(const void* buffer);

/**
 * @brief 
 * @param U 
 */
void unij_unpacker_destroy(unij_unpacker_t* U);

/**
 * @brief 
 * @param U 
 * @return 
 */
void* unij_unpacker_peek(unij_unpacker_t* U);

/**
 * @brief 
 * @param U 
 * @param offset 
 * @return 
 */
bool unij_unpacker_seek(unij_unpacker_t* U, int64_t offset);

/**
 * @brief 
 * @param U 
 * @param dest 
 * @param size 
 * @return 
 */
bool unij_unpack(unij_unpacker_t* U, void* dest, size_t size);

/**
 * @brief 
 * @param U 
 * @param pDest 
 * @return 
 */
bool unij_unpack_wstr(unij_unpacker_t* U, unij_wstr_t* dest);

/**
 * @brief Same as \a unij_unpack_wstr, but allocates a copy of the string with a terminating null character.
 * @param U 
 * @param pDest 
 * @return 
 */
bool unij_unpack_wstrdup(unij_unpacker_t* U, unij_wstr_t* dest);

/**
 * @brief 
 * @param U 
 * @param pDest 
 * @return 
 */
bool unij_unpack_wstr(unij_unpacker_t* U, unij_wstr_t* dest);

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_PACKING_H_ */
