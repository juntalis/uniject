/**
 * @file mono_types.h
 * 
 * TODO: Description
 */
#ifndef _MONO_TYPES_H_
#define _MONO_TYPES_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * When embedding, you have to define MONO_ZERO_LEN_ARRAY before including any
 * other Mono header file if you use a different compiler from the one used to
 * build Mono.
 */
#ifndef MONO_ZERO_LEN_ARRAY
#	ifdef __GNUC__
#		define MONO_ZERO_LEN_ARRAY 0
#	else
#		define MONO_ZERO_LEN_ARRAY ANYSIZE_ARRAY
#	endif
#endif

#ifndef MONO_FALSE
#	define MONO_FALSE 0
#endif

#ifndef MONO_TRUE
#	define MONO_TRUE 1
#endif

typedef enum
{
	MONO_VERIFIER_MODE_OFF,
	MONO_VERIFIER_MODE_VALID,
	MONO_VERIFIER_MODE_VERIFIABLE,
	MONO_VERIFIER_MODE_STRICT
} MiniVerifierMode;

typedef enum
{
	MONO_SECURITY_MODE_NONE,
	MONO_SECURITY_MODE_CORE_CLR,
	MONO_SECURITY_MODE_CAS,
	MONO_SECURITY_MODE_SMCS_HACK
} MonoSecurityMode;

typedef enum
{
	MONO_UNHANDLED_POLICY_LEGACY,
	MONO_UNHANDLED_POLICY_CURRENT
} MonoRuntimeUnhandledExceptionPolicy;

typedef struct MonoError MonoError;
typedef struct MonoObject MonoObject;
typedef struct MonoString MonoString;

typedef struct MonoMethod MonoMethod;
typedef struct MonoMethodDesc MonoMethodDesc;
typedef struct MonoMethodSignature MonoMethodSignature;

typedef struct MonoClass MonoClass;
typedef struct MonoType MonoType;
typedef struct MonoArrayType MonoArrayType;
typedef struct MonoGenericParam MonoGenericParam;
typedef struct MonoGenericClass MonoGenericClass;

typedef struct MonoVTable MonoVTable;

typedef struct MonoThread MonoThread;
typedef struct MonoThreadsSync MonoThreadsSync;

typedef struct MonoException MonoException;
typedef struct MonoAssembly MonoAssembly;
typedef struct MonoClassField MonoClassField;
typedef struct MonoDomain MonoDomain;
typedef struct MonoImage MonoImage;
typedef struct MonoArray MonoArray;
typedef struct MonoProperty MonoProperty;
typedef struct MonoReflectionType MonoReflectionType;
typedef struct MonoReflectionAssembly MonoReflectionAssembly;
typedef struct MonoReflectionMethod MonoReflectionMethod;
typedef struct MonoAppDomain MonoAppDomain;
typedef struct MonoCustomAttrInfo MonoCustomAttrInfo;

typedef int32_t gboolean;
typedef uint16_t mono_unichar2;
typedef uint32_t mono_unichar;

struct MonoObject
{
	MonoVTable* vtable;
	MonoThreadsSync* synchronization;
};

struct MonoString 
{
	MonoObject object;
	int32_t length;
	wchar_t chars[MONO_ZERO_LEN_ARRAY];
};

struct MonoReflectionType
{
	MonoObject object;
	MonoType  *type;
};

struct MonoMethod
{
	uint16_t flags;
	uint16_t iflags;
	// ...
};

struct MonoMethodDesc
{
	char *ns;
	char *klass;
	char *name;
	char *args;
	uint32_t num_args;
	int32_t include_namespace, klass_glob, name_glob;
};

typedef void(*vprintf_func)(const char* msg, va_list args);

// /** Handle macros/functions */
// #define TYPED_HANDLE_PAYLOAD_NAME(TYPE) TYPE ## HandlePayload
// #define TYPED_HANDLE_NAME(TYPE) TYPE ## Handle
// #define TYPED_OUT_HANDLE_NAME(TYPE) TYPE ## HandleOut

// #define TYPED_HANDLE_DECL(TYPE) \
	// typedef struct { TYPE *__raw; } TYPED_HANDLE_PAYLOAD_NAME (TYPE) ; \
	// typedef TYPED_HANDLE_PAYLOAD_NAME(TYPE) * TYPED_HANDLE_NAME (TYPE); \
	// typedef TYPED_HANDLE_PAYLOAD_NAME(TYPE) * TYPED_OUT_HANDLE_NAME (TYPE);


// TYPED_HANDLE_DECL(MonoReflectionType);

#ifdef __cplusplus
}
#endif

#endif /* _MONO_TYPES_H_ */
