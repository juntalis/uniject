; Started on x64 shellcode but then realized I didn't need to go this approach.
bits 64

%define PTRSIZE         0x8
%define SZWCHAR         0x2 ; sizeof(wchar_t)
%define u(x)            __utf16__(x)
%define PLACEHOLDER     0XEFBEADDE

entrypoint:
	push rbx
	push rsi
	push rdi
	push rbp
	sub rsp, 0x28
	xor rdx, rdx
	mov rsi, [ gs:rdx + 0x60 ]            ; rsi = [TEB + 0x60] = PEB
	mov rsi, [ rsi + 0x18 ]               ; rsi = [PEB + 0x18] = PEB_LDR_DATA
	mov rsi, [ rsi + 0x10 ]               ; rsi = [PEB_LDR_DATA + 0x18] = LDR_MODULE InLoadOrder[0] (process)
	lodsq                                 ; rax = InLoadOrder[1] (ntdll)
	mov rsi, [ rax ]                      ; rsi = InLoadOrder[2] (kernel32)
	mov rdi, [ rsi + 0x30 ]               ; rdi = [InLoadOrder[2] + 0x30] = kernel32 DllBase
	mov rsi, PLACEHOLDER                  ; ESI = kernel32.LoadLibraryW - kernel32.base_address (LoadLibraryW RVA)
	add rdi, rsi                          ; rdi = kernel32.LoadLibraryW
	call rdi
	add rsp, 0x28
	pop rbp
	pop rdi
	pop rsi
	pop rbx
	ret

alignb 4

dll_path: dw u('injected_dll'), 0
