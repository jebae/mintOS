[BITS 32]

; export function name to be enabled in C
global readCPUID, switchAndExecute64bitKernel

SECTION .text

; PARAMS: DWORD eax, DWORD* pEAX, DWORD* pEBX, DWORD* pECX, DWORD* pEDX
readCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi

	mov eax, dword [ebp + 8]	; assign value of param eax
	cpuid	; cpuid command returns value to eax, ebx, ecx, edx

	mov esi, dword [ebp + 12]
	mov dword [esi], eax

	mov esi, dword [ebp + 16]
	mov dword [esi], ebx

	mov esi, dword [ebp + 20]
	mov dword [esi], ecx

	mov esi, dword [ebp + 24]
	mov dword [esi], edx

	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

; PARAMS: none
switchAndExecute64bitKernel:

	; set CR4 PAE bit 1, OSXMMEXCPT bit 1, OSFXSR bit 1
	mov eax, cr4
	or eax, 0x620	; PAE (5), OSXMMEXCPT (10), OSFXSR (9) to 1
	mov cr4, eax

	; set CR3 PML4 address
	mov eax, 0x100000
	mov cr3, eax

	; set IA32_EFER LME bit 1
	mov ecx, 0xC0000080	; 0xC0000080 means IA32_EFER MSR register
	rdmsr
	or eax, 0x0100	; eax register role as lower 32bit of IA32_EFER MSR register
	wrmsr

	; set CR0 register and activate paging, caching
	mov eax, cr0
	or eax, 0xE000000E	; set NW(29), CD(30), PG(31), TS(3), EM(2), MP(1) to 1
	xor eax, 0x60000004	; set NW, CD, EM to 0
	mov cr0, eax

	jmp 0x08:0x200000	; set CS segment selector to IA-32e code segment descriptor
										; move to 0x200000(2MB) where IA-32e kernel start
	jmp $
