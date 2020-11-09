[BITS 64]

SECTION .text

extern Main

START:
	mov ax, 0x10	; 0x10 means IA-32e data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; init stack
	mov ss, ax
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8
	call Main	; call C Main

	jmp $
