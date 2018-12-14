/**
 * @file mono_api.h
 * 
 * TODO: Description
 */
#ifndef _MONO_API_H_
#define _MONO_API_H_
#pragma once

#include "mono_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool mono_api_init(const unij_wstr_t* mono_path);
void mono_enable_debugging(void);

#define MONO_API(RET, NAME, ...) \
RET ( * NAME )( __VA_ARGS__ );
#include "mono_api.inl"

#ifdef __cplusplus
}
#endif

#endif /* _MONO_API_H_ */
