/**
 * @file data_test.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"
#include <uniject.h>
#include <uniject/data.h>
#include <uniject/module.h>
#include <uniject/process.h>
#include <uniject/utility.h>

/**
 * @brief Application-specific display of messages to user.
 * In order to give the application/DLL full control over the display of messages and critical failure process, we leave
 * this and ::UNIJ_AbortProcessImpl as undefined external symbols.
 * @param[in] uLevel Message urgency level
 * @param[in] pMessage The error message to show.
 */
void unij_show_message_impl(unij_level_t uLevel, const wchar_t* pMessage)
{
	wchar_t* pScratch = (wchar_t*)pMessage;
	wchar_t* pLevel = (wchar_t*)unij_level_name(uLevel);
	wprintf(L"[%s] %s\n", pLevel, pScratch);
}

/**
 * @brief Application-specific termination of our logic.
 * In order to give the application/DLL full control over the display of messages and critical failure process, we leave
 * this and ::UNIJ_ShowUserMessageImpl as undefined external symbols.
 * NOTE: In the case of the loader DLLs, this shouldn't actually terminate the process. Instead, it should abort the
 * loader logic, then cleanup and unload the loader DLL.
 * @param[in] dwError System error code 
 */
void unij_abort_impl(DWORD dwError)
{
	wprintf(L"Exiting process with code: 0x%08X", (unsigned int)dwError);
	ExitProcess(dwError);
}

static VOID DumpBuffer(LPCWSTR pName, PBYTE pBuffer, SIZE_T szBuffer)
{
	FILE* fOuput = _wfopen(pName, L"wb");
	assert(fOuput != NULL);
	fwrite((const void*)pBuffer, 1, szBuffer, fOuput);
	fflush(fOuput);
	fclose(fOuput);
}

UNIJ_NOINLINE void* CDECL alloc_handler(void* unused, size_t szBytes)
{
	PVOID pResult = malloc(szBytes);
	assert(pResult != NULL);
	memset(pResult, 0, szBytes);
	return pResult;
}

UNIJ_NOINLINE void CDECL free_handler(void* unused, void* ptr)
{
	if(ptr != NULL) {
		free(ptr);
	}
}

typedef struct
{
	bool bool_field;
	uint16_t u16_field;
	uint32_t u32_field;
	uint64_t u64_field;
	size_t size_field;
	unij_wstr_t Mono;
	unij_wstr_t Assembly;
	unij_wstr_t ClassName;
	unij_wstr_t MethodName;
} TestStruct;

static LPCWSTR bool_to_string(bool bValue)
{
	static CONST WCHAR sTRUE[] = L"TRUE";
	static CONST WCHAR sFALSE[] = L"FALSE";
	return bValue ? sTRUE : sFALSE;
}

static void print_wstr(const wchar_t* name, unij_wstr_t* value)
{
	wprintf(L"  %s = L\"%.*s\" [%hu bytes]\n", name, (int)value->length, value->value, value->length);
}

static void dump_params(LPCWSTR pLabel, TestStruct* pParams)
{
	wprintf(L"Params: %s\n", pLabel);
	wprintf(L"  bool_field = %s\n", bool_to_string(pParams->bool_field));
	wprintf(L"  u16_field = %hu\n", pParams->u16_field);
	wprintf(L"  u32_field = 0x%08X\n", pParams->u32_field);
	wprintf(L"  u64_field = %llu\n", pParams->u64_field);
	wprintf(L"  size_field = %zu\n", pParams->size_field);
	print_wstr(L"Mono", &(pParams->Mono));
	print_wstr(L"Assembly", &(pParams->Assembly));
	print_wstr(L"ClassName", &(pParams->ClassName));
	print_wstr(L"MethodName", &(pParams->MethodName));
}

static unij_memprocs_t test_procs = {
	NULL,
	alloc_handler,
	free_handler
};

static unij_wstr_t wsMonoName = {
	ARRAYLEN(L"mono64.dll") - 1,
	L"mono64.dll"
};

static unij_wstr_t wsAssemblyName = {
	ARRAYLEN(L"FakeAssembly.dll") - 1,
	L"FakeAssembly.dll"
};

static unij_wstr_t wsClassName = {
	ARRAYLEN(L"FakeAssemblyLoader") - 1,
	L"FakeAssemblyLoader"
};

static unij_wstr_t wsMethodName = {
	ARRAYLEN(L"LetsGetItStarted") - 1,
	L"LetsGetItStarted"
};

/*
typedef struct
{
	bool bool_field;
	uint16_t u16_field;
	uint32_t u32_field;
	uint64_t u64_field;
	size_t size_field;
	unij_wstr_t Mono;
	unij_wstr_t Assembly;
	unij_wstr_t ClassName;
	unij_wstr_t MethodName;
} TestStruct;
 */

int wmain(int argc, wchar_t *argv[])
{
	unij_packer_t* P;
	unij_unpacker_t* U;
	const void* pBuffer = NULL;
	SIZE_T szBuffer = 0;
	unij_wstr_t sysdir;
	wchar_t lcbuffer[MAX_PATH+1] = L"";
	TestStruct oUnpacking = {0};
	TestStruct oPacking = {
		true,
		300,
		0xDEADBEEF,
		343597383680,
		36864,
		wsMonoName,
		wsAssemblyName,
		wsClassName,
		wsMethodName
	};
	
	if(!unij_init()) return 1;
	
	dump_params(L"=> Packing", &oPacking);
	wprintf(L"\n\n");
	
	P = unij_packer_create(&test_procs);;
	assert(P != NULL);
	unij_reserve_type(P, bool);
	unij_reserve_type(P, uint16_t);
	unij_reserve_type(P, uint32_t);
	unij_reserve_type(P, uint64_t);
	unij_reserve_type(P, size_t);
	unij_reserve_wstr(P, oPacking.Mono);
	unij_reserve_wstr(P, oPacking.Assembly);
	unij_reserve_wstr(P, oPacking.ClassName);
	unij_reserve_wstr(P, oPacking.MethodName);
	unij_packer_commit(P);
	
#	define TRYPACK(P,FLD) \
	wprintf(L"Packing %s..\n", UNIJ_WSTRINGIFY(FLD) ); \
	if( !unij_pack_val( (P), (FLD) ) ) \
		return 1
	
#	define TRYPACKSTR(P,FLD) \
	wprintf(L"Packing %s..\n", UNIJ_WSTRINGIFY(FLD) ); \
	if( !unij_pack_wstr( (P), (FLD) ) ) \
		return 1
	
	TRYPACK(P,oPacking.bool_field);
	TRYPACK(P,oPacking.u16_field);
	TRYPACK(P,oPacking.u32_field);
	TRYPACK(P,oPacking.u64_field);
	TRYPACK(P,oPacking.size_field);
	TRYPACKSTR(P,oPacking.Mono);
	TRYPACKSTR(P,oPacking.Assembly);
	TRYPACKSTR(P,oPacking.ClassName);
	TRYPACKSTR(P,oPacking.MethodName);
	
	pBuffer = (PBYTE)unij_packer_buffer(P);
	szBuffer = (SIZE_T)unij_packer_size(P);
	DumpBuffer(L"packed.bin", (PBYTE)pBuffer, szBuffer);
	
	wprintf(L"Successfully packed data structure into %zu bytes! sizeof(struct) = %zu.\n\n\n", szBuffer, sizeof(oPacking));
	
	U = unij_unpacker_create(pBuffer);
#	define TRYUNPACK(U,FLD) \
	wprintf(L"Unpacking %s..\n", UNIJ_WSTRINGIFY(FLD) ); \
	if( !unij_unpack_val( (U), &(FLD) ) ) \
		return 1

#	define TRYUNPACKSTR(U,FLD) \
	wprintf(L"Unpacking %s..\n", UNIJ_WSTRINGIFY(FLD) ); \
	if( !unij_unpack_wstr( (U), &(FLD) ) ) \
		return 1
	
	TRYUNPACK(U,oUnpacking.bool_field);
	TRYUNPACK(U,oUnpacking.u16_field);
	TRYUNPACK(U,oUnpacking.u32_field);
	TRYUNPACK(U,oUnpacking.u64_field);
	TRYUNPACK(U,oUnpacking.size_field);
	TRYUNPACKSTR(U,oUnpacking.Mono);
	TRYUNPACKSTR(U,oUnpacking.Assembly);
	TRYUNPACKSTR(U,oUnpacking.ClassName);
	TRYUNPACKSTR(U,oUnpacking.MethodName);
	
	dump_params(L"<= Unpacking", &oUnpacking);
	
	
	unij_unpacker_destroy(U);
	unij_packer_destroy(P);
	
	sysdir = unij_system32_dir();
	wprintf(L"Before: %s\n", sysdir.value);
	unij_simplify_path(lcbuffer, sysdir.value, (size_t)sysdir.length);
	wprintf(L"After: %s\n", lcbuffer);
	
	unij_shutdown();
	return 0;
}
