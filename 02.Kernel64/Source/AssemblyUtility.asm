[BITS 64]

SECTION .text

global inPortByte, outPortByte, loadGDTR, loadTR, loadIDTR
global enableInterrupt, disableInterrupt, readRFLAGS
global readTSC
global switchContext, hlt

hlt:
	hlt
	hlt
	ret

; PARAM: port number
inPortByte:
	push rdx
	mov rdx, rdi	; rdi is first parameter
	mov rax, 0
	in al, dx	; dx register has port number data in 1 byte.
						; in [dest] [port] command read data from [port] and save in [dest].
						; al register role as return value of function
						; al, ax, eax register can be used to read data from port
						; with 1, 2, 4 byte individually
	pop rdx
	ret

; PARAM: port number, data
outPortByte:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi	; rsi is second parameter
	out dx, al	; al is used because only 1 byte required

	pop rax
	pop rdx
	ret

loadGDTR:
	lgdt [rdi]
	ret

loadTR:
	ltr di
	ret

loadIDTR:
	lidt [rdi]
	ret

enableInterrupt:
	sti
	ret

disableInterrupt:
	cli
	ret

readRFLAGS:
	pushfq	; cannot read RFLAGS register directly, so use stack
	pop rax	; rax is return value of function
	ret

readTSC:
	push rdx
	rdtsc	; save timestamp counter to rdx, rax
	shl rdx, 32	; rdx << 32
	or rax, rdx
	pop rdx
	ret

%macro SAVE_CONTEXT 0
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds	; cannot push ds and es segment selector directly,
							; so use ax register temporally
	push rax
	mov ax, es
	push rax
	push fs
	push gs
%endmacro

%macro LOAD_CONTEXT 0
	pop gs
	pop fs
	pop rax
	mov es, ax
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

; PARAM: current context, next context
switchContext:
	push rbp
	mov rbp, rsp

	pushfq	; temporally save RFLAGS register cause cmp can change RFLAGS data
	cmp rdi, 0	; rdi register have first parameter
							; check if current context is null
	je .LoadContext
	popfq

	push rax	; use rax as variable

	; SS
	mov ax, ss
	mov qword[rdi + (23 * 8)], rax

	; RSP
	mov rax, rbp
	add rax, 16	; skip push rbp, return address
	mov qword[rdi + (22 * 8)], rax

	; RFLAGS
	pushfq
	pop rax
	mov qword[rdi + (21 * 8)], rax

	; CS
	mov ax, cs
	mov qword[rdi + (20 * 8)], rax

	; RIP
	; save return address
	mov rax, qword[rbp + 8]
	mov qword[rdi + (19 * 8)], rax

	pop rax
	pop rbp

	; set stack pointer to next of 5 register (SS, RSP, ...)
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)

	SAVE_CONTEXT

.LoadContext:
	mov rsp, rsi	; rsi is second parameter
								; set stack pointer to next task
	LOAD_CONTEXT
	iretq	; pop 5 registers (SS, RSP, ...)
