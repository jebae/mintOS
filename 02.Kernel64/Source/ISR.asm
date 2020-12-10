[BITS 64]

SECTION .text

extern commonExceptionHandler, commonInterruptHandler, keyboardHandler
extern timerHandler, deviceNotAvailableHandler, HDDhandler

; exception ISR
global ISRDivideError, ISRDebug, ISRNMI, ISRBreakPoint, ISROverflow
global ISRBoundRangeExceeded, ISRInvalidOpcode, ISRDeviceNotAvailable, ISRDoubleFault
global ISRCoprocessorSegmentOverrun, ISRInvalidTSS, ISRSegmentNotPresent
global ISRStackSegmentFault, ISRGeneralProtection, ISRPageFault, ISR15
global ISRFPUError, ISRAlignmentCheck, ISRMachineCheck, ISRSIMDError, ISRETCException

; interrupt ISR
global ISRTimer, ISRKeyboard, ISRSlavePIC, ISRSerial2, ISRSerial1, ISRParallel2
global ISRFloppy, ISRParallel1, ISRRTC, ISRReserved, ISRNotUsed1, ISRNotUsed2
global ISRMouse, ISRCoprocessor, ISRHDD1, ISRHDD2, ISRETCInterrupt

%macro SAVE_CONTEXT 0
	push rbp
	mov rbp, rsp
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

	; change segment selector
	mov ax, 0x10	; kernel data segment
	mov ds, ax
	mov es, ax
	mov gs, ax
	mov fs, ax
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

; Error handler
ISRDivideError:
	SAVE_CONTEXT
	mov rdi, 0	; exception number parameter for handler
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRDebug:
	SAVE_CONTEXT
	mov rdi, 1
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRNMI:
	SAVE_CONTEXT
	mov rdi, 2
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRBreakPoint:
	SAVE_CONTEXT
	mov rdi, 3
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISROverflow:
	SAVE_CONTEXT
	mov rdi, 4
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRBoundRangeExceeded:
	SAVE_CONTEXT
	mov rdi, 5
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRInvalidOpcode:
	SAVE_CONTEXT
	mov rdi, 6
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRDeviceNotAvailable:
	SAVE_CONTEXT
	mov rdi, 7
	call deviceNotAvailableHandler
	LOAD_CONTEXT
	iretq

ISRDoubleFault:
	SAVE_CONTEXT
	mov rdi, 8
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8	; remove error code from stack
	iretq

ISRCoprocessorSegmentOverrun:
	SAVE_CONTEXT
	mov rdi, 9
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRInvalidTSS:
	SAVE_CONTEXT
	mov rdi, 10
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8
	iretq

ISRSegmentNotPresent:
	SAVE_CONTEXT
	mov rdi, 11
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8
	iretq

ISRStackSegmentFault:
	SAVE_CONTEXT
	mov rdi, 12
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8
	iretq

ISRGeneralProtection:
	SAVE_CONTEXT
	mov rdi, 13
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8
	iretq

ISRPageFault:
	SAVE_CONTEXT
	mov rdi, 14
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8
	iretq

ISR15:
	SAVE_CONTEXT
	mov rdi, 15
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRFPUError:
	SAVE_CONTEXT
	mov rdi, 16
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRAlignmentCheck:
	SAVE_CONTEXT
	mov rdi, 17
	mov rsi, qword [rbp + 8]	; error code
	call commonExceptionHandler
	LOAD_CONTEXT
	add rsp, 8
	iretq

ISRMachineCheck:
	SAVE_CONTEXT
	mov rdi, 18
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRSIMDError:
	SAVE_CONTEXT
	mov rdi, 19
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

ISRETCException:
	SAVE_CONTEXT
	mov rdi, 20
	call commonExceptionHandler
	LOAD_CONTEXT
	iretq

; Interrupt handler
ISRTimer:
	SAVE_CONTEXT
	mov rdi, 32
	call timerHandler
	LOAD_CONTEXT
	iretq

ISRKeyboard:
	SAVE_CONTEXT
	mov rdi, 33
	call keyboardHandler
	LOAD_CONTEXT
	iretq

ISRSlavePIC:
	SAVE_CONTEXT
	mov rdi, 34
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRSerial2:
	SAVE_CONTEXT
	mov rdi, 35
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRSerial1:
	SAVE_CONTEXT
	mov rdi, 36
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRParallel2:
	SAVE_CONTEXT
	mov rdi, 37
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRFloppy:
	SAVE_CONTEXT
	mov rdi, 38
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRParallel1:
	SAVE_CONTEXT
	mov rdi, 39
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRRTC:
	SAVE_CONTEXT
	mov rdi, 40
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRReserved:
	SAVE_CONTEXT
	mov rdi, 41
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRNotUsed1:
	SAVE_CONTEXT
	mov rdi, 42
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRNotUsed2:
	SAVE_CONTEXT
	mov rdi, 43
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRMouse:
	SAVE_CONTEXT
	mov rdi, 44
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRCoprocessor:
	SAVE_CONTEXT
	mov rdi, 45
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq

ISRHDD1:
	SAVE_CONTEXT
	mov rdi, 46
	call HDDhandler
	LOAD_CONTEXT
	iretq

ISRHDD2:
	SAVE_CONTEXT
	mov rdi, 47
	call HDDhandler
	LOAD_CONTEXT
	iretq

ISRETCInterrupt:
	SAVE_CONTEXT
	mov rdi, 48
	call commonInterruptHandler
	LOAD_CONTEXT
	iretq
