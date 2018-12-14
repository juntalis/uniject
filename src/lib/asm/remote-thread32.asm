bits 32

%define SZWCHAR         0x2 ; sizeof(wchar_t)
%define u(x)            __utf16__(x) 
%define PLACEHOLDER     0XEFBEADDE

;get_eip:
	;mov eax, [esp]
	;ret
entrypoint:
	push ebp
	mov ebp, esp
	push esi
	push edi
	;call get_eip
	;add eax, dll_path - $           ; eax = dll_path
	mov eax, [ebp+8]                 ; eax = dll_path
	push eax                         ; stack: dll_path, edi, esi, ebp
	xor ecx, ecx                     ; ecx = 0
	mov esi, [ fs:ecx + 0x30 ]       ; esi = &(PEB) ([FS:0x30])
	mov esi, [ esi + 0x0C ]          ; esi = PEB->Ldr
	mov esi, [ esi + 0x1C ]          ; esi = PEB->Ldr.InInitOrder (first module)
.next_module:
	mov ebp, [ esi + 0x08 ]          ; ebp = InInitOrder[X].base_address
	mov edi, [ esi + 0x20 ]          ; edi = InInitOrder[X].module_name (unicode string)
	mov esi, [ esi ]                 ; esi = InInitOrder[X].flink (next module)
	cmp [ edi + 12*SZWCHAR ], cx     ; modulename[12] == 0 ? strlen("kernel32.dll") == 12
	jne .next_module                 ; No: try next module.
	mov edi, ebp                     ; edi = kernel32.base_address
	mov esi, PLACEHOLDER             ; esi = kernel32.LoadLibraryW - kernel32.base_address (LoadLibraryW RVA)
	add edi, esi                     ; edi = kernel32.LoadLibraryW
	call edi                         ; stack: dll_path, edi, esi, ebp
	pop edi                          ; stack: edi, esi, ebp
	pop esi                          ; stack: esi, ebp
	pop ebp                          ; stack: ebp
	retn 4

align 4, db 0

dll_path: dw u('injected_dll'), 0
