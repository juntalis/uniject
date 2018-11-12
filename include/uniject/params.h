/**
 * @file uniject/params.h
 * @brief Defines the injection parameters and exposes the functions for packing/unpacking them from shared memory.
 */
#ifndef _UNIJECT_PARAMS_H_
#define _UNIJECT_PARAMS_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 */
struct unij_params
{
	uint32_t pid;
	
	// Flags
	bool debugging : 1;
	bool new_thread : 1;
	
	// Strings
	unij_wstr_t mono_name;
	unij_wstr_t assembly_name;
	unij_wstr_t class_name;
	unij_wstr_t method_name;
	unij_wstr_t log_path;
};

typedef struct unij_params unij_params_t;

// Forward declaration
typedef struct unij_packer unij_packer_t;
typedef struct unij_unpacker unij_unpacker_t;

void unij_reserve_params(unij_packer_t* P, const unij_params_t* data);
bool unij_pack_params(unij_packer_t* P, const unij_params_t* data);
bool unij_unpack_params(unij_unpacker_t* U, unij_params_t* dest);

#ifdef __cplusplus
}
#endif

#endif /* _UNIJECT_PARAMS_H_ */
