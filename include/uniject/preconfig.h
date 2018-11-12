/**
 * @file uniject/preconfig.h
 * 
 * TODO: Description
 */
#ifndef _UNIJECT_PRECONFIG_H_
#define _UNIJECT_PRECONFIG_H_
#pragma once

/* Windows Version */
// - Target Windows XP and higher.
// - Throw a compile-time error if a lower platform is detected.
#if !defined(WINVER)
#	define WINVER 0x0501
#elif (WINVER < 0x0501)
#	error Windows XP is currently the lowest version of Windows supported by this project.
#endif

/* Force UNICODE character set */
#ifdef _MBCS
#	undef _MBCS
#endif

#ifndef _UNICODE
#	define _UNICODE 1
#endif

#ifndef UNICODE
#	define UNICODE 1
#endif

// Speed up build process with minimal headers.
#ifndef WIN32_LEAN_AND_MEAN
#	define VC_EXTRALEAN 1
#	define WIN32_LEAN_AND_MEAN 1
#endif

// Compiler Detection
#if defined(__clang__)
#	define UNIJ_CC_CLANG 1
#elif defined(_MSC_VER) && !defined(RC_INVOKED)
#	define UNIJ_CC_MSVC 1
#elif (defined(__MINGW32__) || defined(__GNUC__))
#	define UNIJ_CC_GNU 1
#endif

// Architecture Detection
#if defined(__x86_64__) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_X64)
#	define UNIJ_BITS 64
#	define UNIJ_ARCH_X64
#	ifndef _WIN64
#		define _WIN64
#	endif
#elif defined(__i386__) || defined(_M_IX86)
#	define UNIJ_BITS 32
#	define UNIJ_ARCH_X86
#endif

#ifndef UNIJ_BITS
#	error Could not detect platform architecture.
#endif

// MSVC-specific flags
#ifdef UNIJ_CC_MSVC
/* Warning Suppression */
//	Insecure function usage warnings
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS 1
#	endif
//	More depreciation warnings
#	pragma warning(disable:4996)

/*	WinSDK Version */
#	include <SDKDDKVer.h>
#endif

#endif /* _UNIJECT_PRECONFIG_H_ */
