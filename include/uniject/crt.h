/**
 * @file uniject/crt.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Overrides and helpers for avoiding usage of the MS CRT library.
 */
#ifndef _UNIJECT_CRT_H_
#define _UNIJECT_CRT_H_
#pragma once

#ifndef _UNIJECT_H_
#	include <uniject.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RtlFillMemory
#	pragma push_macro("RtlFillMemory")
#	undef RtlFillMemory
#endif

#ifdef RtlZeroMemory
#	pragma push_macro("RtlZeroMemory")
#	undef RtlZeroMemory
#endif

#ifdef RtlMoveMemory
#	pragma push_macro("RtlMoveMemory")
#	undef RtlMoveMemory
#endif

#ifdef RtlCopyMemory
#	pragma push_macro("RtlCopyMemory")
#	undef RtlCopyMemory
#endif

#ifdef RtlEqualMemory
#	pragma push_macro("RtlEqualMemory")
#	undef RtlEqualMemory
#endif

#ifdef RtlCompareMemory
#	pragma push_macro("RtlCompareMemory")
#	undef RtlCompareMemory
#endif

typedef ULONG LOGICAL;

UNIJ_DLLIMP void WINAPI RtlFillMemory(PVOID, SIZE_T, BYTE);
UNIJ_DLLIMP void WINAPI RtlZeroMemory(PVOID, SIZE_T);
UNIJ_DLLIMP void WINAPI RtlMoveMemory(PVOID, const VOID*, SIZE_T);
UNIJ_DLLIMP LOGICAL WINAPI RtlEqualMemory(const VOID*, const VOID*, SIZE_T);
UNIJ_DLLIMP SIZE_T WINAPI RtlCompareMemory(const VOID*, const VOID*, SIZE_T);

#ifdef UNIJ_ARCH_X64
UNIJ_DLLIMP void WINAPI RtlCopyMemory(PVOID, const VOID*, SIZE_T);
#else
#define RtlCopyMemory RtlMoveMemory 
#endif

#ifdef __cplusplus
}
#endif

#endif /* _UNIJECT_CRT_H_ */
