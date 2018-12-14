/**
 * @file loader.c
 * @author Charles Grunwald <ch@rles.rocks>
 * 
 * TODO: Allow having the user target the main mono thread without knowing its id beforehand.
 */
#include "pch.h"
#include "error.h"
#include "mono_api.h"

#include <uniject/params.h>
#include <uniject/injector.h>

typedef struct hijack_data hijack_data_t;

struct hijack_data
{
	HANDLE event;
	unij_error_t status;
	const uniject_t* ctx;
};

DEFINE_STATIC_WSTR(DEFAULT_CLASSNAME, L"Loader");
DEFINE_STATIC_WSTR(DEFAULT_METHOD, L"Initialize");

static UNIJ_INLINE char* build_method_desc(unij_cstr_t* cls, unij_cstr_t* method)
{
	char* descstr = (char*)unij_alloc((size_t)(cls->length + method->length) + 2);
	assert(descstr != NULL);
	lstrcpyA(descstr, cls->value);
	lstrcatA(descstr, ":");
	lstrcatA(descstr, method->value);
	return descstr;
}

static UNIJ_INLINE unij_wstr_t* default_to(unij_wstr_t* pvalue, unij_wstr_t* pdefault)
{
	return unij_is_empty(pvalue) ? pdefault : pvalue;
}

// At this point in the process, the root appdomain has been acquired with the working thread attached to it.
static unij_error_t mono_main(const uniject_t* ctx, MonoDomain *domain)
{
	unij_error_t result = UNIJ_ERROR_SUCCESS;
	
	MonoImage *image;
	MonoMethod* method;
	MonoAssembly* assembly;
	MonoMethodDesc* desc = NULL;
	char* descstr = NULL;
	unij_cstr_t aname, cname, mname;
	unij_wstr_t *pcname, *pmname;
	unij_params_t* params = unij_get_params((uniject_t*)ctx);
	
	pcname = default_to(&params->class_name, &DEFAULT_CLASSNAME);
	pmname = default_to(&params->method_name, &DEFAULT_METHOD);
	
	aname = unij_wstrtocstr(&params->assembly_path);
	if(unij_is_empty(&aname)) {
		return unij_get_fatal_error().unij_code;
	}
	
	cname = unij_wstrtocstr(pcname);
	if(unij_is_empty(&cname)) {
		return unij_get_fatal_error().unij_code;
	}
	
	mname = unij_wstrtocstr(pmname);
	if(unij_is_empty(&mname)) {
		return unij_get_fatal_error().unij_code;
	}
	
	if(params->debugging)
		mono_enable_debugging();
	
	assembly = mono_domain_assembly_open(domain, aname.value);
	if(!assembly) {
		result = UNIJ_ERROR_MONO;
		unij_show_error_message(L"Failed call to mono_domain_assembly_open!");
		goto cleanup;
	}
	
	image = mono_assembly_get_image(assembly);
	if(!image) {
		result = UNIJ_ERROR_MONO;
		unij_show_error_message(L"Failed call to mono_assembly_get_image!");
		goto cleanup;
	}
	
	descstr = build_method_desc(&cname, &mname);
	if(!descstr) {
		result = UNIJ_ERROR_INTERNAL;
		unij_show_error_message(L"Failed to build mono desc string!");
		goto cleanup;
	}
	
	desc = mono_method_desc_new(descstr, true);
	if(!desc) {
		result = UNIJ_ERROR_MONO;
		unij_show_error_message(L"Failed call to mono_method_desc_new!");
		goto cleanup;
	}
	
	method = mono_method_desc_search_in_image(desc, image);
	if(!method) {
		result = UNIJ_ERROR_MONO;
		unij_show_error_message(L"Failed locate class/method with call to mono_method_desc_search_in_image!");
		goto cleanup;
	}
	
	mono_runtime_invoke(method, 0, 0, 0);
	
cleanup:
	
	if(desc != NULL) {
		mono_method_desc_free(desc);
		desc = NULL;
	}
	
	if(descstr != NULL) {
		unij_free((void*)descstr);
		descstr = NULL;
	}
	
	unij_cstrfree(&aname);
	unij_cstrfree(&cname);
	unij_cstrfree(&mname);
	
	return result;
}

static unij_error_t remote_thread_setup(const uniject_t* ctx)
{
	bool result;
	MonoThread* thread;
	MonoDomain *domain = mono_get_root_domain();
	if(domain == NULL) {
		unij_show_error_message(L"Loader failed to acquire the root appdomain!");
		return UNIJ_ERROR_MONO;
	}
	
	thread = mono_thread_attach(domain);
	if(thread == NULL) {
		unij_show_error_message(L"Loader failed to acquire the root appdomain!");
		return UNIJ_ERROR_MONO;
	}
	
	result = mono_main(ctx, domain);
	mono_thread_detach(thread);
	
	return result;
}

static void hijack_complete(hijack_data_t* data, unij_error_t status)
{
	HANDLE event = data->event;
	
	data->status = status;
	if(status != UNIJ_ERROR_SUCCESS)
		unij_abort(status);
	
	data->event = NULL;
	SetEvent(event);
	CloseHandle(event);
}

static void CDECL hijacked_entrypoint(hijack_data_t* data)
{
	MonoThread* thread;
	MonoDomain *domain;
	
	// Ensure this thread is attached to the app domain.
	thread = mono_thread_current();
	if(thread == NULL) {
		uint32_t thread_id = (uint32_t)GetCurrentThreadId();
		unij_show_error_message(L"Specified thread (%u) is not attached to an app domain!", thread_id);
		hijack_complete(data, UNIJ_ERROR_PID);
		return;
	}
	
	// App domain
	domain = mono_get_root_domain();
	if(domain == NULL) {
		unij_show_error_message(L"Loader failed to acquire the root appdomain!");
		hijack_complete(data, UNIJ_ERROR_MONO);
		return;
	}
	
	hijack_complete(data, mono_main(data->ctx, domain));
}

static unij_error_t hijacked_thread_setup(const uniject_t* ctx, uint32_t tid)
{
	unij_error_t result;
	HANDLE process, duplicate;
	HANDLE wait_handles[] = { NULL, NULL };
	hijack_data_t data = { NULL, UNIJ_ERROR_SUCCESS, ctx, };
	
	// First we need to ensure that we can open the target thread
	wait_handles[0] = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)tid);
	if(wait_handles[0] == NULL) {
		unij_fatal_call(OpenThread);
		return UNIJ_ERROR_LASTERROR;
	}
	
	// Next, create our event for synchronization with the hijacked threead
	wait_handles[1] = CreateEventW(NULL, FALSE, FALSE, NULL);
	if(wait_handles[1] == NULL) {
		CloseHandle(wait_handles[0]);
		unij_fatal_call(CreateEvent);
		return UNIJ_ERROR_LASTERROR;
	}
	
	// Duplicate the event handle
	process = GetCurrentProcess();
	if(!DuplicateHandle(process, wait_handles[1], process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		result = UNIJ_ERROR_LASTERROR;
		unij_fatal_call(DuplicateHandle);
		goto cleanup;
		
	}
	
	// do the actual hijacking
	data.event = duplicate;
	if(!unij_hijack_thread(wait_handles[0], (unij_hijack_fn)hijacked_entrypoint, (void*)&data)) {
		result = unij_get_fatal_error().unij_code;
		CloseHandle(duplicate);
	} else {
		// Listen on the event and the thread, just in case the thread somehow aborts before our event gets set.
		WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);
		result = data.status; // TODO: Verify the thread still exists to ensure data.status being correct
	}
	
cleanup:
	CloseHandle(wait_handles[0]);
	CloseHandle(wait_handles[1]);
	return result;
}

unij_error_t loader_main(void)
{
	uniject_t* ctx;
	const unij_params_t* params;
	unij_error_t result = UNIJ_ERROR_SUCCESS;
	ctx = unij_loader_open();
	if(ctx == NULL) {
		unij_show_message(UNIJ_LEVEL_INFO, L"ctx = NULL");
		return UNIJ_ERROR_INTERNAL;
	}
	
	params = unij_get_params(ctx);
	if(params == NULL) {
		unij_fatal_error(UNIJ_ERROR_INTERNAL, L"Params = NULL!");
		return UNIJ_ERROR_INTERNAL;
	}
	
	if(!mono_api_init(&params->mono_path)) {
		unij_close(ctx);
		return UNIJ_ERROR_INTERNAL;
	}
	
	if(params->tid == 0) {
		result = remote_thread_setup(ctx);
	} else {
		result = hijacked_thread_setup(ctx, params->tid);
	}
	
	unij_close(ctx);
	return result;
}
