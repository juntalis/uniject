bits 64

%define PLACEHOLDER     0XEFBEADDE
%define u(x)            __utf16__(x)

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
	sub rsp, 0x28
	lea rcx,[rel callback_param]
	call [rel callback_fn]
	add rsp, 0x28
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
	jmp qword [rel original_rip]

align 16, db 0

original_rip: dq PLACEHOLDER
callback_fn: dq PLACEHOLDER
callback_param: dq PLACEHOLDER