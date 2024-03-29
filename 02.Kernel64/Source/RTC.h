#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

#define RTC_CMOS_ADDRESS				0x70
#define RTC_CMOS_DATA						0x71
#define RTC_ADDRESS_SECOND			0x00
#define RTC_ADDRESS_MINUTE			0x02
#define RTC_ADDRESS_HOUR				0x04
#define RTC_ADDRESS_DAYOFWEEK		0x06
#define RTC_ADDRESS_DAYOFMONTH	0x07
#define RTC_ADDRESS_MONTH				0x08
#define RTC_ADDRESS_YEAR				0x09

#define RTC_BCD_TO_BIN(x) ((((x) >> 4) * 10) + ((x) & 0x0F))

void readRTCTime(BYTE* hour, BYTE* minute, BYTE* second);
void readRTCDate(WORD* year, BYTE* month, BYTE* dayOfMonth, BYTE* dayOfWeek);
char* convertDayOfWeekToString(BYTE dayOfWeek);

#endif
