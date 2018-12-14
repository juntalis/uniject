/**
 * @file base_private.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for base_private.h
 */
#ifndef _BASE_PRIVATE_H_
#define _BASE_PRIVATE_H_
#pragma once

#include <uniject/base.h>
#include <uniject/ipc.h>
#include <uniject/packing.h>
#include <uniject/process.h>

#ifdef __cplusplus
extern "C" {
#endif

// Struct declarations

enum unij_role
{
	ROLE_INJECTOR = 0,
	ROLE_LOADER
};

typedef enum unij_role unij_role_t;
typedef struct unijector unijector_t;

/**
 * @brief IPC context
 */
struct unij_ipc
{
	bool custom;
	const wchar_t* name;
	HANDLE file_handle;
	void* mapped_view;
	unij_memprocs_t memprocs;
	unij_reserve_fn reserve_fn;
	union
	{
		unij_pack_fn pack_fn;
		unij_unpack_fn unpack_fn;
	};
	// Doubles as access to the originating uniject_t* when standard IPC
	unij_role_t* role;
};

// Private uniject context structure
struct uniject
{
	unij_role_t role;
	unij_ipc_t ipc;
	unij_params_t params;
};

struct unijector
{
	uniject_t u;
	unij_wstr_t loader;
	unij_process_t* process;
};

// Function prototypes

bool unij_memory_init(void);

#ifdef __cplusplus
};
#endif

#endif /* _BASE_PRIVATE_H_ */
