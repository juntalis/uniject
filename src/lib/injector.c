/**
 * @file injector.c
 * @author Charles Grunwald <ch@rles.rocks>
 */
#include "pch.h"

typedef struct injection_code injection_code_t;
typedef void*(CDECL* code_render_fn)(injection_code_t* pcode);

struct injection_code
{
	uint32_t ip_off;
	uint32_t loadlib_off;
	uint32_t entrypoint_off;
	
	size_t template_size;
	const uint8_t* template;
	code_render_fn render_fn;
};

static const uint8_t TMPL_THREAD_CONTEXT32[] = {
	                                    // get_eip:
	0x8B, 0x04, 0x24,                   //   mov eax, [esp+0]
	0xC3,                               //   retn
	                                    // entrypoint:
	0x68, 0x00, 0x00, 0x00, 0x00,       //   push original_ip
	0x9C,                               //   pushf
	0x60,                               //   pusha
	0xE8, 0xF0, 0xFF, 0xFF, 0xFF,       //   call get_eip
	0x83, 0xC0, 0x30,                   //   add eax, dll_path - $; "injected_dll"
	0x50,                               //   push eax
	0x31, 0xC9,                         //   xor ecx, ecx
	0x64, 0x8B, 0x71, 0x30,             //   mov esi, fs:[ecx+30h]
	0x8B, 0x76, 0x0C,                   //   mov esi, [esi+0Ch]
	0x8B, 0x76, 0x1C,                   //   mov esi, [esi+1Ch]
	                                    // next_module:
	0x8B, 0x6E, 0x08,                   //   mov ebp, [esi+8]
	0x8B, 0x7E, 0x20,                   //   mov edi, [esi+20h]
	0x8B, 0x36,                         //   mov esi, [esi]
	0x66, 0x39, 0x4F, 0x18,             //   cmp [edi+18h], cx
	0x75, 0xF2,                         //   jnz short next_module
	0x89, 0xEF,                         //   mov edi, ebp
	0x81, 0xC7, 0x00, 0x00, 0x00, 0x00, //   add edi, loadlib_rva
	0xFF, 0xD7,                         //   call edi
	0x31, 0xC0,                         //   xor eax, eax
	0x61,                               //   popa
	0x9D,                               //   popf
	0xC3,                               //   retn
	                                    // alignment:
	0x00,                               //   db 0h
	0x00,                               //   db 0h
	0x00,                               //   db 0h
};

#define TMPL_THREAD_CONTEXT32_OFFSET_0 0x05
#define TMPL_THREAD_CONTEXT32_OFFSET_1 0x32
#define TMPL_THREAD_CONTEXT32_ENTRYPOINT 0x5

static void* CDECL render_context_injection32(injection_code_t* pcode);

static const injection_code_t CODE_THREAD_CONTEXT32 = {
	TMPL_THREAD_CONTEXT32_OFFSET_0,
	TMPL_THREAD_CONTEXT32_OFFSET_1,
	TMPL_THREAD_CONTEXT32_ENTRYPOINT,
	sizeof(TMPL_THREAD_CONTEXT32),
	TMPL_THREAD_CONTEXT32,
	render_context_injection32
};

static const uint8_t TMPL_REMOTE_THREAD32[] = {
	                                    // get_eip:
	0x8B, 0x04, 0x24,                   //   mov eax, [esp+0]
	0xC3,                               //   retn
	                                    // entrypoint:
	0x55,                               //   push ebp
	0x89, 0xE5,                         //   mov ebp, esp
	0x56,                               //   push esi
	0x57,                               //   push edi
	0xE8, 0xF2, 0xFF, 0xFF, 0xFF,       //   call get_eip
	0x83, 0xC0, 0x2E,                   //   add eax, dll_path - $; "injected_dll"
	0x50,                               //   push eax
	0x31, 0xC9,                         //   xor ecx, ecx
	0x64, 0x8B, 0x71, 0x30,             //   mov esi, fs:[ecx+30h]
	0x8B, 0x76, 0x0C,                   //   mov esi, [esi+0Ch]
	0x8B, 0x76, 0x1C,                   //   mov esi, [esi+1Ch]
	                                    // next_module:
	0x8B, 0x6E, 0x08,                   //   mov ebp, [esi+8]
	0x8B, 0x7E, 0x20,                   //   mov edi, [esi+20h]
	0x8B, 0x36,                         //   mov esi, [esi]
	0x66, 0x39, 0x4F, 0x18,             //   cmp [edi+18h], cx
	0x75, 0xF2,                         //   jnz short next_module
	0x89, 0xEF,                         //   mov edi, ebp
	0x81, 0xC7, 0x00, 0x00, 0x00, 0x00, //   add edi, loadlib_rva
	0xFF, 0xD7,                         //   call edi
	0x5F,                               //   pop edi
	0x5E,                               //   pop esi
	0x5D,                               //   pop ebp
	0xC2, 0x04, 0x00,                   //   retn 4
};

#define TMPL_REMOTE_THREAD32_OFFSET_0 0x30
#define TMPL_REMOTE_THREAD32_ENTRYPOINT 0x5

static void* CDECL render_remote_injection32(injection_code_t* pcode);

static const injection_code_t CODE_REMOTE_THREAD32 = {
	0,
	TMPL_REMOTE_THREAD32_OFFSET_0,
	TMPL_THREAD_CONTEXT32_ENTRYPOINT,
	sizeof(TMPL_REMOTE_THREAD32),
	TMPL_REMOTE_THREAD32,
	render_remote_injection32
};

#ifdef UNIJ_ARCH_X64

static const uint8_t TMPL_ANY64[] = {
	                                // original_ip:
	0,0,0,0,0,0,0,0,                //   dq 0
	                                // loadlib_ptr:
	0,0,0,0,0,0,0,0,                //   dq 0
	                                // entrypoint:
	0x9C,                           //   pushfq
	0x50,                           //   push rax
	0x51,                           //   push rcx
	0x52,                           //   push rdx
	0x53,                           //   push rbx
	0x55,                           //   push rbp
	0x56,                           //   push rsi
	0x57,                           //   push rdi
	0x41,0x50,                      //   push r8
	0x41,0x51,                      //   push r9
	0x41,0x52,                      //   push r10
	0x41,0x53,                      //   push r11
	0x41,0x54,                      //   push r12
	0x41,0x55,                      //   push r13
	0x41,0x56,                      //   push r14
	0x41,0x57,                      //   push r15
	0x48,0x83,0xEC,0x28,            //   sub rsp,28h
	0x48,0x8D,0x0D,0x29,0,0,0,      //   lea rcx,[dll_path-$]
	0xFF,0x15,0xCF,0xFF,0xFF,0xFF,  //   call [loadlib_ptr]
	0x48,0x83,0xC4,0x28,            //   add rsp,28h
	0x41,0x5F,                      //   pop r15
	0x41,0x5E,                      //   pop r14
	0x41,0x5D,                      //   pop r13
	0x41,0x5C,                      //   pop r12
	0x41,0x5B,                      //   pop r11
	0x41,0x5A,                      //   pop r10
	0x41,0x59,                      //   pop r9
	0x41,0x58,                      //   pop r8
	0x5F,                           //   pop rdi
	0x5E,                           //   pop rsi
	0x5D,                           //   pop rbp
	0x5B,                           //   pop rbx
	0x5A,                           //   pop rdx
	0x59,                           //   pop rcx
	0x58,                           //   pop rax
	0x9D,                           //   popfq
	0xFF,0x25,0xAF,0xFF,0xFF,0xFF,  //   jmp [original_ip]
	                                // alignment:
	0,                              //   db 0
};

#define TMPL_ANY64_OFFSET_0 0x00
#define TMPL_ANY64_OFFSET_1 0x08
#define TMPL_ANY64_ENTRYPOINT 0x10

static void* CDECL render_context_injection64(injection_code_t* pcode);
static void* CDECL render_remote_injection64(injection_code_t* pcode);

static const injection_code_t CODE_THREAD_CONTEXT64 = {
	TMPL_ANY64_OFFSET_0,
	TMPL_ANY64_OFFSET_1,
	TMPL_ANY64_ENTRYPOINT,
	sizeof(TMPL_ANY64),
	TMPL_ANY64,
	render_context_injection64
};

static const injection_code_t CODE_REMOTE_THREAD64 = {
	TMPL_ANY64_OFFSET_0,
	TMPL_ANY64_OFFSET_1,
	TMPL_ANY64_ENTRYPOINT,
	sizeof(TMPL_ANY64),
	TMPL_ANY64,
	render_remote_injection64
};

#endif

static void* render_context_injection32(injection_code_t* pcode)
{
	return NULL;
}

static void* render_remote_injection32(injection_code_t* pcode)
{
	
}

static void* CDECL render_context_injection64(injection_code_t* pcode)
{
	
}

static void* CDECL render_remote_injection64(injection_code_t* pcode)
{
	
}
