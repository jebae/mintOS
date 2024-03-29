[BITS 64]

SECTION .text

global inPortByte, outPortByte, loadGDTR, loadTR, loadIDTR, inPortWord, outPortWord
global enableInterrupt, disableInterrupt, readRFLAGS
global readTSC
global switchContext, hlt, checkLockAndSet
global initFPU, saveFPUContext, loadFPUContext, setTS, clearTS

hlt:
	hlt
	hlt
	ret

; PARAM: port number(rdi)
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

; PARAM: port number(rdi)
inPortWord:
	push rdx
	mov rdx, rdi
	mov rax, 0
	in ax, dx
	pop rdx
	ret

; PARAM: port number(rdi), data(rsi)
outPortByte:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi	; rsi is second parameter
	out dx, al	; al is used because only 1 byte required

	pop rax
	pop rdx
	ret

; PARAM: port number(rdi), data(rsi)
outPortWord:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi
	out dx, ax

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

; PARAM: current context(rdi), next context(rsi)
switchContext:
	push rbp
	mov rbp, rsp

	pushfq	; temporally save RFLAGS register cause cmp can change RFLAGS data
	cmp rdi, 0	; rdi is first parameter
							; check if current context is null
	je .LoadContext
	popfq

	push rax	; use rax as variable

	; SS
	mov ax, ss
	mov qword[rdi + (23 * 8)], rax

	; RSP
	; current stack status
	; --- data before call switchContext			---
	; --- return address after switchContext	---
	; --- rbp																	--- <- bp
	; --- rax																	--- <- sp
	mov rax, rbp
	add rax, 16	; skip rbp, return address
	mov qword[rdi + (22 * 8)], rax	; set rsp to location before switchContext

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

; PARAM: dest(rdi), compare(rsi), src(rdx)
checkLockAndSet:
	mov rax, rsi
	lock cmpxchg byte[rdi], dl	; lock command makes other process to not access memory
															; cmpxchg compare rax and first parameter
															; if same, assign second parameter to first parameter
															; if different, assign first parameter to rax
	je .SUCCESS

.NOT_SAME:
	mov rax, 0x00
	ret

.SUCCESS:
	mov rax, 0x01
	ret

initFPU:
	finit
	ret

; PARAM: context buffer address(rdi)
saveFPUContext:
	fxsave [rdi]
	ret

; PARAM: context buffer address(rdi)
loadFPUContext:
	fxrstor [rdi]
	ret

setTS:
	push rax
	mov rax, cr0
	or rax, 0x08	; set TS bit 1
	mov cr0, rax
	pop rax
	ret

clearTS:
	clts
	ret
