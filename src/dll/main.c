/**
 * @file main.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include "error.h"
#include "loader.h"

// Used to determine if the currently executing code is running within a hijacked thread or the original injected
// thread.
static uint32_t tls_landmark = 0;

bool unij_loader_thread(void)
{
	return tls_landmark == 0 || TlsGetValue(tls_landmark) == NULL;
}

static UNIJ_INLINE bool loader_init(void* instance)
{
	bool result = true;
	tls_landmark = (uint32_t)TlsAlloc();
	if(tls_landmark == TLS_OUT_OF_INDEXES) {
		unij_fatal_call(TlsAlloc);
		result = false;
	} else if(!TlsSetValue(tls_landmark, instance)) {
		unij_fatal_call(TlsSetValue);
		TlsFree(tls_landmark);
		result = false;
	} else {
		unij_error_init();
	}
	return result;
}

static UNIJ_INLINE void loader_shutdown()
{
	TlsFree(tls_landmark);
	unij_error_shutdown();
}

static BOOL process_status(unij_error_t status)
{
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	BOOL result = TRUE;
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE)instance);
			unij_show_message(UNIJ_LEVEL_INFO, L"Loader entry point hit!");
			if(loader_init(instance)) {
				unij_error_t status = loader_main();
				result = process_status(status);
			} else {
				result = FALSE;
				unij_show_message(UNIJ_LEVEL_ERROR, L"Loader failed during initialization logic!");
			}
			break;
		case DLL_PROCESS_DETACH:
			loader_shutdown();
			break;
	}
	return result;
}
