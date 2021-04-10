/**
 * @file ctx.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "base_private.h"
#include "process_private.h"
#include "error_private.h"
#include <uniject/injector.h>
#include <uniject/utility.h>

#define ENSURE_CTX(CTX) \
	(!unij_fatal_null(CTX))

#define ENSURE_LOADER(CTX) \
	( (uniject_t*)ctx_ensure_role((void*)(CTX), ROLE_LOADER, true, __FUNCTIONW__) )

#define ENSURE_INJECTOR(CTX) \
	( (unijector_t*)ctx_ensure_role((void*)(CTX), ROLE_INJECTOR, true, __FUNCTIONW__) )

#define UNIJECTOR(CTX) \
	( (unijector_t*)ctx_ensure_role((void*)(CTX), ROLE_INJECTOR, false, __FUNCTIONW__) )

static UNIJ_INLINE const wchar_t* ctx_role_text(unij_role_t role)
{
	switch(role)
	{
		case ROLE_LOADER:
			return L"loader";
		case ROLE_INJECTOR:
			return L"injector";
		default:
			return L"<INTERNAL ERROR>";
	}
}

static void* ctx_ensure_role(uniject_t* ctx, unij_role_t role, bool error, const wchar_t* caller)
{
	uniject_t* result = NULL;
	if(!unij_fatal_null2(ctx, caller)) {
		if(ctx->role == role) {
			result = ctx;
		} else if(error) {
			unij_fatal_error(
				UNIJ_ERROR_OPERATION,
				L"%s requires a uniject context with a '%s' role assigned!",
				ctx_role_text(role)
			);
		}
	}
	return (void*)result;
}

bool unij_init(void)
{
	return unij_memory_init();
}

static uniject_t* ctx_alloc(unij_process_t* process, size_t size, bool resolve_mono)
{
	uniject_t* ctx = NULL;
	unij_wstr_t* mono_path = NULL;
	if(!unij_init())
		return ctx;
		
	if(resolve_mono && process != NULL) {
		mono_path = unij_process_get_mono_path(process);
		if(mono_path == NULL) {
			return NULL;
		}
	}
	
	ctx = (uniject_t*)unij_alloc(size);
	if(ctx == NULL) {
		unij_fatal_alloc();
		return NULL;
	}
	
	if(process == NULL) {
		ctx->role = ROLE_LOADER;
		ctx->params.pid = (uint32_t)GetCurrentProcessId();
	} else {
		unijector_t* ext = (unijector_t*)ctx;
		assert(size == sizeof(unijector_t));
		ext->process = process;
		ctx->role = ROLE_INJECTOR;
		ctx->params.pid = process->pid;
		if(resolve_mono) {
			ctx->params.mono_path = unij_wstrdup(mono_path);
		}
	}
	
	if(!unij_ipc_open(ctx)) {
		unij_free((void*)ctx);
		ctx = NULL;
	}
	
	return ctx;
}

uniject_t* unij_injector_open(uint32_t pid)
{
	uniject_t* result = NULL;
	
	// Initialize process object
	unij_process_t* process =  unij_process_open(pid);
	if(process == NULL) {
		return result;
	}
	
	// Allocate our ctx struct and initialize IPC context
	result = ctx_alloc(process, sizeof(unijector_t), true);
	if(result == NULL) {
		unij_process_close(process);
		return result;
	}
	
	return result;
}

uniject_t* unij_injector_assign(HANDLE process_handle)
{
	uniject_t* result = NULL;
	
	// Initialize process object
	unij_process_t* process =  unij_process_assign(process_handle);
	if(process == NULL) {
		return result;
	}
	
	// Allocate our ctx struct and initialize IPC context
	result = ctx_alloc(process, sizeof(unijector_t), true);
	if(result == NULL) {
		unij_process_close(process);
		return result;
	}
	
	return result;
}

uniject_t* unij_injector_open_params(const unij_params_t* params)
{
	bool needs_resolve;
	uniject_t* result = NULL;
	unij_wstr_t* mono_path = NULL;
	unij_process_t* process = NULL;
	
	// verify params ptr
	if(unij_fatal_null(params)) {
		return NULL;
	}
	
	// Initialize process object
	process =  unij_process_open(params->pid);
	if(process == NULL) {
		return result;
	}
	
	// Determine if we need to resolve mono path
	needs_resolve = unij_is_empty(&params->mono_path);
	
	// Allocate our ctx struct and initialize IPC context
	result = ctx_alloc(process, sizeof(unijector_t), needs_resolve);
	if(result == NULL) {
		unij_process_close(process);
		return result;
	}
	
	// Copy over the params.
	result->params.tid = params->tid;
	result->params.debugging = params->debugging;
	result->params.log_path = unij_wstrdup(&params->log_path);
	result->params.class_name = unij_wstrdup(&params->class_name);
	result->params.method_name = unij_wstrdup(&params->method_name);
	result->params.assembly_path = unij_wstrdup(&params->assembly_path);
	if(!needs_resolve) {
		result->params.mono_path = unij_wstrdup(&params->mono_path);
	}
	return result;
}

uniject_t* unij_loader_open(void)
{
	// Allocate our ctx struct and initialize IPC context
	uniject_t* ctx = ctx_alloc(NULL, sizeof(uniject_t), false);
	if(ctx != NULL && !unij_ipc_unpack(&ctx->ipc, (void*)&(ctx->params))) {
		unij_close(ctx);
		ctx = NULL;
	}
	return ctx;
}

static void ctx_free_params(uniject_t* ctx)
{
	unij_params_t* params = &ctx->params;
	unijector_t* injector = UNIJECTOR(ctx);
	unij_wstrfree(&params->assembly_path);
	unij_wstrfree(&params->class_name);
	unij_wstrfree(&params->method_name);
	unij_wstrfree(&params->log_path);
	unij_wstrfree(&params->mono_path);
	if(injector != NULL) {
		unij_wstrfree(&injector->loader);
		RtlZeroMemory((void*)params, sizeof(unij_params_t));
	}
}

void unij_close(uniject_t* ctx)
{
	if(ctx != NULL) {
		unijector_t* injector = UNIJECTOR(ctx);
		if(injector != NULL)
			unij_process_close(injector->process);
		
		if(ctx->ipc.name != NULL)
			unij_ipc_close(&ctx->ipc);
		
		ctx_free_params(ctx);
		unij_free((void*)ctx);
	}
}

bool unij_inject(uniject_t* ctx)
{
	unijector_t* injector = ENSURE_INJECTOR(ctx);
	if(injector == NULL) return false;
	if(!unij_ipc_pack(&ctx->ipc, (const void*)&ctx->params)) {
		return false;
	}
	return unij_inject_loader_ex(injector->process, &injector->loader);
}

unij_params_t* unij_get_params(uniject_t* ctx)
{
	return ENSURE_CTX(ctx) ? &ctx->params : NULL;
}

#define IMPL_PARAM_GETTER(TYPE,PARAM) \
	TYPE unij_get_##PARAM (uniject_t* ctx) \
	{ \
		return ENSURE_CTX(ctx) ? ctx->params. PARAM : (TYPE)(0); \
	}

#define IMPL_WSTR_PARAM_GETTER(PARAM) \
	unij_wstr_t* unij_get_##PARAM (uniject_t* ctx) \
	{ \
		return ENSURE_CTX(ctx) ? &( ctx->params. PARAM ) : NULL; \
	}

#define IMPL_PARAM_SETTER(TYPE,PARAM) \
	void unij_set_##PARAM (uniject_t* ctx, TYPE value) \
	{ \
		if( !ENSURE_INJECTOR(ctx) ) return; \
		ctx->params. PARAM = value; \
	}

#define IMPL_WSTR_PARAM_SETTER(PARAM) \
	void unij_set_##PARAM (uniject_t* ctx, unij_wstr_t* value) \
	{ \
		if( !ENSURE_INJECTOR(ctx) ) return; \
		if(!unij_is_empty(&ctx->params. PARAM )) { \
			unij_wstrfree(&ctx->params . PARAM); \
		} \
		ctx->params. PARAM = unij_wstrdup(value); \
	}

IMPL_PARAM_GETTER(uint32_t, pid);
IMPL_PARAM_GETTER(uint32_t, tid);
IMPL_PARAM_GETTER(bool, debugging);

IMPL_WSTR_PARAM_GETTER(mono_path);
IMPL_WSTR_PARAM_GETTER(assembly_path);
IMPL_WSTR_PARAM_GETTER(class_name);
IMPL_WSTR_PARAM_GETTER(method_name);
IMPL_WSTR_PARAM_GETTER(log_path);

IMPL_PARAM_SETTER(uint32_t, tid);
IMPL_PARAM_SETTER(bool, debugging);

IMPL_WSTR_PARAM_SETTER(assembly_path);
IMPL_WSTR_PARAM_SETTER(class_name);
IMPL_WSTR_PARAM_SETTER(method_name);
IMPL_WSTR_PARAM_SETTER(log_path);

void unij_set_loader_path(uniject_t* ctx, unij_wstr_t* value)
{
	unijector_t* injector = ENSURE_INJECTOR(ctx);
	if(ctx == NULL) return;
	injector->loader = unij_wstrdup(value);
}
