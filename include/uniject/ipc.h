/**
 * @file uniject/ipc.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Interprocess communication layer
 */
#ifndef _UNIJECT_IPC_H_
#define _UNIJECT_IPC_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 */
enum unij_role
{
	UNIJ_ROLE_INJECTOR = 0,
	UNIJ_ROLE_LOADER
};

typedef enum unij_role unij_role_t;

/**
 * @brief IPC context declaration
 */
typedef struct unij_ipc unij_ipc_t;

// Forward declaration
typedef struct unij_packer unij_packer_t;
typedef struct unij_unpacker unij_unpacker_t;

typedef void (CDECL *unij_reserve_fn)(unij_packer_t* P, const void* data);
typedef bool (CDECL *unij_pack_fn)(unij_packer_t* P, const void* data);
typedef bool (CDECL *unij_unpack_fn)(unij_unpacker_t* U, void* dest);

// IPC context with injector role
unij_ipc_t* unij_ipc_injector_create(uint32_t pid, const wchar_t* key);

// IPC context with loader role
unij_ipc_t* unij_ipc_loader_create(const wchar_t* key);

/**
 * @brief Injector only: Wait on the loader to complete.
 * TODO: timeout?
 * @param[in] ctx IPC context 
 * @return 
 */
bool unij_ipc_injector_wait(unij_ipc_t* ctx);

/**
 * @brief Injector only: Wait on the loader to complete.
 * TODO: timeout?
 * @param[in] ctx IPC context 
 * @return 
 */
bool unij_ipc_loader_notify(unij_ipc_t* ctx);

/**
 * @brief Destroy either context. DO NOT CALL before calling unij_ipc_injector_wait
 * @param ctx 
 */
void unij_ipc_destroy(unij_ipc_t* ctx);

/* For overriding default params struct */

/**
 * @brief Injector only: override the default data reserve handler
 * @param ctx 
 * @param fn 
 */
void unij_ipc_set_reserve_fn(unij_ipc_t* ctx, unij_reserve_fn fn);

/**
 * @brief Injector only: override the default data packing handler
 * @param ctx 
 * @param fn 
 */
void unij_ipc_set_pack_fn(unij_ipc_t* ctx, unij_pack_fn fn);

/**
 * @brief Loader only: override the default data unpacking handler
 * @param ctx 
 * @param fn 
 */
void unij_ipc_set_unpack_fn(unij_ipc_t* ctx, unij_unpack_fn fn);

/**
 * @brief Injector only: will use the set reverse/pack functions or fallback to those declared in params.h
 * @param ctx 
 * @param data 
 * @return 
 */
bool unij_ipc_pack(unij_ipc_t* ctx, const void* data);

/**
 * @brief Loader only: will use the set unpack function or fallback to the one declared in params.h
 * @param ctx 
 * @param data 
 * @return 
 */
bool unij_ipc_unpack(unij_ipc_t* ctx, void* dest);

#ifdef __cplusplus
};
#endif

#endif /* _UNIJECT_IPC_H_ */
