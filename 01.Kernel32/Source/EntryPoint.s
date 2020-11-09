[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax, 0x1000
	mov ds, ax
	mov es, ax

	; activate A20 gate
	mov ax, 0x2401	; 0x2401 means activate A20 gate using BIOS
	int 0x15	; interrupt BIOS

	jc .A20_GATE_ERROR	; handle interrupt error
	jmp .A20_GATE_SUCCESS

; try system control port when activating A20 with BIOS fail
.A20_GATE_ERROR:
	in al, 0x92	; read system control port(0x92) 
	or al, 0x02	; OR operate to A20 gate bit (A20 gate = bit index 1)
	and al, 0xFE	; prevent system reset (system reset = bit index 0)
	out 0x92, al	; set system control port

.A20_GATE_SUCCESS:
	cli	; prevent interrupt
	lgdt [GDTR]	; setting GDT table

	; switch to protected mode with setting CR0 register
	mov eax, 0x4000003B	; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0
											; MP=1, PE=1
	mov cr0, eax
	jmp dword 0x18:(PROTECT_MODE - $$ + 0x10000)	; 0x18 means
																								; kernel code descriptor in GDT

[BITS 32]
PROTECT_MODE:
	mov ax, 0x20	; 0x20 means data segment descriptor
								; in protect mode, use segment with descriptor
								; not register directly
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; init stack
	mov ss, ax	; use data segment as stack segment
	mov esp, 0xFFFE
	mov ebp, 0xFFFE

	push (SWITCH_SUCCESS_MESSAGE - $$ + 0x10000)
	push 2
	push 0
	call PRINT_MESSAGE
	add esp, 12

	jmp dword 0x18:0x10200	; move to 0x10200 which contains C kernel

; PARAMS: x, y, string
PRINT_MESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx

	mov eax, dword [ebp + 12]
	mov esi, 160
	mul esi
	mov edi, eax

	mov eax, dword [ebp + 8]
	mov esi, 2
	mul esi
	add edi, eax

	mov esi, dword [ebp + 16]

.MESSAGE_LOOP:
	mov cl, byte [esi]
	cmp cl, 0
	je .MESSAGE_END

	mov byte [edi + 0xB8000], cl
	add esi, 1
	add edi, 2
	jmp .MESSAGE_LOOP

.MESSAGE_END:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret

align 8, db 0	; align data below with 8 byte
dw 0x0000	; inutil zeros to make GDTR 8 byte

GDTR:
	dw GDT_END - GDT - 1	; size of GDT table
	dd (GDT - $$ + 0x10000)	; start address of GDT table

GDT:
	; segment descriptor (8 byte)
	NULL_Descriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00

	; IA-32e code segment descriptor
	IA_32e_CODE_DESCRIPTOR:
		dw 0xFFFF	; size of segment [15:0]
		dw 0x0000	; base address of memory [31:16]
		db 0x00	; base address of memory [39:32]
		db 0x9A	; P=1, DPL=0, type=Execute/Read [47:40]
		db 0xAF	; G=1, D=0, L=1, size of segment [55:48]
		db 0x00	; base address of memory [63:56]

	; data segment descriptor
	IA_32e_DATA_DESCRIPTOR:
		dw 0xFFFF	; size of segment [15:0]
		dw 0x0000	; base address of memory [31:16]
		db 0x00	; base address of memory [39:32]
		db 0x92	; P=1, DPL=0, type=Read/Write [47:40]
		db 0xAF	; G=1, D=0, L=1, size of segment [55:48]
		db 0x00	; base address of memory [63:56]

	; code segment descriptor
	CODE_DESCRIPTOR:
		dw 0xFFFF	; size of segment [15:0]
		dw 0x0000	; base address of memory [31:16]
		db 0x00	; base address of memory [39:32]
		db 0x9A	; P=1, DPL=0, type=Execute/Read [47:40]
		db 0xCF	; G=1, D=1, L=0, size of segment [55:48]
		db 0x00	; base address of memory [63:56]

	; data segment descriptor
	DATA_DESCRIPTOR:
		dw 0xFFFF	; size of segment [15:0]
		dw 0x0000	; base address of memory [31:16]
		db 0x00	; base address of memory [39:32]
		db 0x92	; P=1, DPL=0, type=Read/Write [47:40]
		db 0xCF	; G=1, D=1, L=0, size of segment [55:48]
		db 0x00	; base address of memory [63:56]

GDT_END:

SWITCH_SUCCESS_MESSAGE: db 'Switch To Protected Mode Success', 0

times 512 - ($ - $$) db 0x00	; assign zero to rest
