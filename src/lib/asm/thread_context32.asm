BITS 32

%define PTRSIZE         0x4
%define SZWCHAR         0x2 ; sizeof(wchar_t)
%define u(x)            __utf16__(x) 
%define PLACEHOLDER     0XEFBEADDE

get_eip:
	mov eax, [esp]
	ret
entrypoint:
	push dword PLACEHOLDER           ; Store original context (Will be replaced in C)
	pushf
	pusha
	call get_eip
	add eax, dll_path - $            ; EAX = L"injected_dll\0"
	push dword eax                   ; Stack: L"injected_dll\0", original context
	xor ecx, ecx                     ; ECX = 0
; Find base address of kernel32.dll. This code should work on Windows 5.0-7.0
	mov esi, [ fs:ecx + 0x30 ]       ; ESI = &(PEB) ([FS:0x30])
	mov esi, [ esi + (PTRSIZE * 3) ] ; ESI = PEB->Ldr
	mov esi, [ esi + 0x1C ]          ; ESI = PEB->Ldr.InInitOrder (first module)
next_module:
	mov ebp, [ esi + 0x08 ]          ; EBP = InInitOrder[X].base_address
	mov edi, [ esi + 0x20 ]          ; EDI = InInitOrder[X].module_name (unicode string)
	mov esi, [ esi ]                 ; ESI = InInitOrder[X].flink (next module)
	cmp [ edi + 12*SZWCHAR ], cx     ; modulename[12] == 0 ? strlen("kernel32.dll") == 12
	jne next_module                  ; No: try next module.
	mov edi, dword ebp               ; EDI = kernel32.base_address
	add edi, dword PLACEHOLDER       ; EDI = kernel32.LoadLibraryW (LoadLibraryW offset found at runtime)
	call dword edi                   ; Stack: L"injected_dll\0", EBP, caller
	xor eax,eax                      ;
	popa                             ; Restore original context.
	popf
	ret
padding0:
	db 0, 0, 0
dll_path:
	db u('injected_dll'), 0x0, 0x0
