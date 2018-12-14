#include <windows.h>
#include <stdlib.h>

typedef int(WINAPI* TMsgBox)(PVOID hwnd, LPCWSTR text, LPCWSTR caption, UINT flags);

static BOOL ShowSuccess(void)
{
	BOOL bResult = FALSE;
	TMsgBox MsgBox = NULL;
	HMODULE hUser32 = NULL;
	hUser32 = LoadLibraryW(L"user32.dll");
	if(hUser32 == NULL) return bResult;
	*((FARPROC*)&MsgBox) = GetProcAddress(hUser32, "MessageBoxW");
	if(MsgBox == NULL)
		goto cleanup;
	
	MsgBox(NULL, L"Successfully injected DLL!", L"Success", 0x00000040);
	bResult = TRUE;
	
cleanup:
	if(hUser32 != NULL) {
		FreeLibrary(hUser32);
	}
	return bResult;
}


BOOL WINAPI _DllMainCRTStartup(HINSTANCE const instance, DWORD const reason, LPVOID const reserved)
{
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			return ShowSuccess();
	}
	return TRUE;
}