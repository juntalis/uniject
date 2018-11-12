; Started on x64 shellcode but then realized I didn't need to go this approach.
BITS 64

%define PTRSIZE         0x8
%define SZWCHAR         0x2 ; sizeof(wchar_t)
%define u(x)            __utf16__(x) 
%define PLACEHOLDER     0XEFBEADDE

original_rip:
	dq 0x0
loadlib_rvaa:
	dq 0x0
entrypoint:
	pushfq
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	xor rcx, rcx
	mov rsi, [ gs:rcx + 0x60 ]            ; rsi = [TEB + 0x60] = PEB
	mov esi, [ rsi + (PTRSIZE * 3) ]      ; rsi = [PEB + 0x18] = PEB_LDR_DATA
	mov esi, [ rsi + 0x10 ]               ; rsi = [PEB_LDR_DATA + 0x18] = LDR_MODULE InLoadOrder[0] (process)
	lodsd                                 ; rax = InLoadOrder[1] (ntdll)
	mov esi, [rax]                        ; rsi = InLoadOrder[2] (kernel32)
	                                      ; kernel32/ntdll < 2Gb for compatibility
	mov edi, [rsi + 0x30]                 ; rdi = [InLoadOrder[2] + 0x30] = kernel32 DllBase
	add edi, dword PLACEHOLDER
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
	popfq

