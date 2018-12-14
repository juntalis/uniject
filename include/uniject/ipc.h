/**
 * @file uniject/ipc.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Interprocess communication layer
 * 
 * TODO: Possibly have the reader wait on an event that signals the writer's completion.
 */
#ifndef _UNIJECT_IPC_H_
#define _UNIJECT_IPC_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC context forward declaration
 */
typedef struct unij_ipc unij_ipc_t;

// Forward declarations
typedef struct uniject uniject_t;
typedef struct unij_packer unij_packer_t;
typedef struct unij_unpacker unij_unpacker_t;

typedef void (CDECL *unij_reserve_fn)(unij_packer_t* P, const void* data);
typedef bool (CDECL *unij_pack_fn)(unij_packer_t* P, const void* data);
typedef bool (CDECL *unij_unpack_fn)(unij_unpacker_t* U, void* dest);

// Standard IPC open
bool unij_ipc_open(uniject_t* ctx);

// Custom IPC openers
unij_ipc_t* unij_ipc_writer_open(uint32_t pid, const wchar_t* key);
unij_ipc_t* unij_ipc_reader_open(const wchar_t* key);

/**
 * @brief Destroy IPC context. (works for both standard and custom variations) 
 * @param ctx 
 */
void unij_ipc_close(unij_ipc_t* ipc);

/* For overriding default params struct */

/**
 * @brief Writer only: override the default data reserve handler
 * @param ipc 
 * @param fn 
 */
void unij_ipc_set_reserve_fn(unij_ipc_t* ipc, unij_reserve_fn fn);

/**
 * @brief Writer only: override the default data packing handler
 * @param ipc 
 * @param fn 
 */
void unij_ipc_set_pack_fn(unij_ipc_t* ipc, unij_pack_fn fn);

/**
 * @brief Reader only: override the default data unpacking handler
 * @param ipc 
 * @param fn 
 */
void unij_ipc_set_unpack_fn(unij_ipc_t* ipc, unij_unpack_fn fn);

/**
 * @brief Writer only: will use the set reverse/pack functions or fallback to those declared in params.h
 * @param ipc 
 * @param data 
 * @return 
 */
bool unij_ipc_pack(unij_ipc_t* ipc, const void* data);

/**
 * @brief Reader only: will use the set unpack function or fallback to the one declared in params.h
 * @param ipc 
 * @param data 
 * @return 
 */
bool unij_ipc_unpack(unij_ipc_t* ipc, void* dest);

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_IPC_H_ */
