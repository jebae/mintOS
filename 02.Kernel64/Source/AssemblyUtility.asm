[BITS 64]

SECTION .text

global inPortByte, outPortByte

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
