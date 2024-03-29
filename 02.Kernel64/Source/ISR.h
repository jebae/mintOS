#ifndef __ISR_H__
#define __ISR_H__

void ISRDivideError(void);
void ISRDebug(void);
void ISRNMI(void);
void ISRBreakPoint(void);
void ISROverflow(void);
void ISRBoundRangeExceeded(void);
void ISRInvalidOpcode();
void ISRDeviceNotAvailable(void);
void ISRDoubleFault(void);
void ISRCoprocessorSegmentOverrun(void);
void ISRInvalidTSS(void);
void ISRSegmentNotPresent(void);
void ISRStackSegmentFault(void);
void ISRGeneralProtection(void);
void ISRPageFault(void);
void ISR15(void);
void ISRFPUError(void);
void ISRAlignmentCheck(void);
void ISRMachineCheck(void);
void ISRSIMDError(void);
void ISRETCException(void);

void ISRTimer(void);
void ISRKeyboard(void);
void ISRSlavePIC(void);
void ISRSerial2(void);
void ISRSerial1(void);
void ISRParallel2(void);
void ISRFloppy(void);
void ISRParallel1(void);
void ISRRTC(void);
void ISRReserved(void);
void ISRNotUsed1(void);
void ISRNotUsed2(void);
void ISRMouse(void);
void ISRCoprocessor(void);
void ISRHDD1(void);
void ISRHDD2(void);
void ISRETCInterrupt(void);

#endif
