[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START	; set code segment as 0x07C0
									; 0x07C0 * 16 is address which BIOS copy bootloader from disk
									; and copy

TOTAL_SECTOR_COUNT: dw 0x02	; os image size except boot loader
KERNEL32_SECTOR_COUNT: dw 0x02	; protected mode kernel sectors

START:
	mov ax, 0x07C0
	mov ds, ax	; set data segment as 0x07C0
	mov ax, 0xB800
	mov es, ax	; set video memory address

	; init stack
	mov ax, 0x0000
	mov ss, ax	; ss = stack segment
	mov sp, 0xFFFE	; sp = stack pointer
	mov bp, 0xFFFE	; bp = base pointer = stack bottom

	mov si, 0	; use si as source index of string

.SCREEN_CLEAR_LOOP:
	mov byte [es: si], 0	; set video memory's character
	mov byte [es: si + 1], 0x0A	; set video memory's attribute byte
	add si, 2
	cmp si, 80 * 25 * 2	; initial video memory size is 80 * 25
											; rest 2 means video memory character + attribute

	jl .SCREEN_CLEAR_LOOP

	; call PRINT_MESSAGE with start message
	push START_MESSAGE
	push 0
	push 0
	call PRINT_MESSAGE
	add sp, 6	; remove parameters from stack

	; call PRINT_MESSAGE with image loading message
	push IMAGE_LOADING_MESSAGE
	push 1
	push 0
	call PRINT_MESSAGE
	add sp, 6	; remove parameters from stack

RESET_DISK:
	mov ax, 0	; service number
	mov dl, 0	; drive number (0 = Floppy)
	int 0x13	; interrupt
	jc HANDLE_DISK_ERROR

	mov si, 0x1000	; 0x1000 * 16 is address where os image will be copied
	mov es, si
	mov bx, 0x0000
	mov di, word [TOTAL_SECTOR_COUNT]

READ_DATA:
	cmp di, 0	; until TOTAL_SECTOR_COUNT
	je READ_END
	sub di, 0x1

	; Call BIOS read function
	mov ah, 0x02	; BIOS service number (2 = read sector)
	mov al, 0x01	; number of sector
	mov ch, byte [TRACK_NUMBER]
	mov cl, byte [SECTOR_NUMBER]
	mov dh, byte [HEAD_NUMBER]
	mov dl, 0x00	; drive number (0 = Floppy)
	int 0x13	; interrupt disk I/O service
	jc HANDLE_DISK_ERROR

	add si, 0x0020	; forward 512 byte (0x200)
	mov es, si

	mov al, byte [SECTOR_NUMBER]
	add al, 0x01
	mov byte [SECTOR_NUMBER], al	; increase sector number
	cmp al, 37	; check 18 sectors in one track is read
	jl READ_DATA

	xor byte [HEAD_NUMBER], 0x01	; flip head number
	mov byte [SECTOR_NUMBER], 0x01

	cmp byte [HEAD_NUMBER], 0x00
	jne READ_DATA

	add	byte [TRACK_NUMBER], 0x01	; increase track number
	jmp READ_DATA

READ_END:
	push LOADING_COMPLETE_MESSAGE
	push 1
	push 20
	call PRINT_MESSAGE
	add sp, 6

	jmp 0x1000:0x0000	; execute os

HANDLE_DISK_ERROR:
	push DISK_ERROR_MESSAGE
	push 1
	push 20
	call PRINT_MESSAGE
	add sp, 6

	jmp $

; PARAM: x, y, message string
PRINT_MESSAGE:
	push bp	; push caller's base pointer
	mov bp, sp	; move base pointer to current stack pointer position
							; in this way, parameters pushed by caller can be accessed
							; with offset from current bp

	push es	; to use registers in function
	push si	; save data of registers in stack and restore it when pop
	push di
	push ax
	push cx
	push dx

	mov ax, 0xB800
	mov es, ax

	; calculate video memory address offset with x, y parameters
	mov ax, word [bp + 6]	; access parameter y
	mov si, 160
	mul si
	mov di, ax

	mov ax, word [bp + 4]	; access parameter x
	mov si, 2
	mul si
	add di, ax

	mov si, word [bp + 8]	; access parameter string

.MESSAGE_LOOP:
	mov cl, byte [si]	; cl is assigned with message's character
	cmp cl, 0	; check string end
	je .MESSAGE_END
	mov byte [es:di], cl	; copy character of START_MESSAGE to video memory
	add si, 1
	add di, 2
	jmp .MESSAGE_LOOP

.MESSAGE_END:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret

START_MESSAGE: db 'mintOS Boot Loader Start', 0	; string end with 0
DISK_ERROR_MESSAGE: db 'Disk Error', 0
IMAGE_LOADING_MESSAGE: db 'OS Image Loading...', 0
LOADING_COMPLETE_MESSAGE: db 'Complete', 0

SECTOR_NUMBER: db 0x02	; sector number starts with 2 (1 is used by BIOS)
HEAD_NUMBER: db 0x00
TRACK_NUMBER: db 0x00

times 510 - ($ - $$) db	0x00	; assign zero to rest

db 0x55
db 0xAA
