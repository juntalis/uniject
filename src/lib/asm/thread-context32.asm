bits 32

%define PLACEHOLDER 0XEFBEADDE

get_eip:
	mov eax, [esp]
	ret
entrypoint:
	push dword PLACEHOLDER
	pushf
	pusha
	call get_eip
	lea eax,[eax + callback_fn - $]
	mov edi,[eax]
	mov eax,[eax+4]
	push eax
	call edi
	add esp, 4
	popa
	popf
	ret

align 4, db 0

callback_fn: dd PLACEHOLDER
callback_param: dd PLACEHOLDER
