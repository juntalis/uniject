/**
 * @file params.c
 * 
 * TODO: Probably just be easier to delimit the string fields with null bytes instead of packing/unpacking based on lengths.
 * TODO: Default value fallback should probably be handled somewhere else.
 */
#include "pch.h"
#include <uniject/packing.h>
#include <uniject/params.h>

// Potential bitset values for our "flags"
#define FLAGS_NONE      (0)
#define FLAGS_DEBUGGING (1<<0)
#define FLAGS_NEWTHREAD (1<<1)

void unij_reserve_params(unij_packer_t* P, const unij_params_t* data)
{
	unij_reserve_type(P, uint32_t); // pid
	unij_reserve_type(P, uint32_t); // tid
	unij_reserve_type(P, uint32_t); // flags
	unij_reserve_wstr(P, &data->mono_path);
	unij_reserve_wstr(P, &data->assembly_path);
	unij_reserve_wstr(P, &data->class_name);
	unij_reserve_wstr(P, &data->method_name);
	unij_reserve_wstr(P, &data->log_path);
}

/**
 * @brief Packs bools into a bitset
 */
static UNIJ_INLINE bool unij_pack_flags(unij_packer_t* P, const unij_params_t* data)
{
	uint32_t flags = FLAGS_NONE;
	if(data->debugging)  flags |= FLAGS_DEBUGGING;
	return unij_pack_val(P, flags);
}

bool unij_pack_params(unij_packer_t* P, const unij_params_t* data)
{
	if(!unij_pack_val(P, data->pid)) return false;
	if(!unij_pack_val(P, data->tid)) return false;
	if(!unij_pack_flags(P, data)) return false;
	if(!unij_pack_wstr(P, &data->mono_path)) return false;
	if(!unij_pack_wstr(P, &data->assembly_path)) return false;
	if(!unij_pack_wstr(P, &data->class_name)) return false;
	if(!unij_pack_wstr(P, &data->method_name)) return false;
	if(!unij_pack_wstr(P, &data->log_path)) return false;
	return true;
}

/**
 * @brief Packs all the ``bool`` fields from \a ::unij_params_t into a bitset (for \a unij_params_packed::Flags)
 * @param[in] pSrc Pointer to input params
 */
static UNIJ_INLINE bool unij_unpack_flags(unij_unpacker_t* U, unij_params_t* dest)
{
	uint32_t flags = 0;
	bool result = unij_unpack_val(U, &flags);
	if(result) {
		dest->debugging = (bool)(flags & FLAGS_DEBUGGING ? 1 : 0);
	}
	return result;
}

bool unij_unpack_params(unij_unpacker_t* U, unij_params_t* dest)
{
	if(!unij_unpack_val(U, &(dest->pid))) return false;
	if(!unij_unpack_val(U, &(dest->tid))) return false;
	if(!unij_unpack_flags(U, dest)) return false;
	if(!unij_unpack_wstrdup(U, &(dest->mono_path))) return false;
	if(!unij_unpack_wstrdup(U, &(dest->assembly_path))) return false;
	if(!unij_unpack_wstrdup(U, &(dest->class_name))) return false;
	if(!unij_unpack_wstrdup(U, &(dest->method_name))) return false;
	if(!unij_unpack_wstrdup(U, &(dest->log_path))) return false;
	return true;
}
