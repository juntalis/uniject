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

typedef void(*mono_api_error_fn)(uint32_t code, const char* message);

#define MONO_API(RET, NAME, ...) \
typedef RET ( *imp_ ## NAME )( __VA_ARGS__ );
#include "mono_api.inl"

bool mono_api_init(const wchar_t* filepath);
void mono_api_set_error_handler(mono_api_error_fn handler);
void mono_api_report_error(uint32_t code, const char* message);

#define MONO_API(RET, NAME, ...) \
extern imp_ ## NAME NAME;
#include "mono_api.inl"

#ifdef __cplusplus
}
#endif

#endif /* _MONO_API_H_ */
