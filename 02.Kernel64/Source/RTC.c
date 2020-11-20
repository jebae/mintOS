#include "RTC.h"
#include "AssemblyUtility.h"

void readRTCTime(BYTE* hour, BYTE* minute, BYTE* second)
{
	BYTE data;

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_HOUR);
	data = inPortByte(RTC_CMOS_DATA);
	*hour = RTC_BCD_TO_BIN(data);

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_MINUTE);
	data = inPortByte(RTC_CMOS_DATA);
	*minute = RTC_BCD_TO_BIN(data);

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_SECOND);
	data = inPortByte(RTC_CMOS_DATA);
	*second = RTC_BCD_TO_BIN(data);
}

void readRTCDate(WORD* year, BYTE* month, BYTE* dayOfMonth, BYTE* dayOfWeek)
{
	BYTE data;

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_YEAR);
	data = inPortByte(RTC_CMOS_DATA);
	*year = RTC_BCD_TO_BIN(data) + 2000;

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_MONTH);
	data = inPortByte(RTC_CMOS_DATA);
	*month = RTC_BCD_TO_BIN(data);

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_DAYOFMONTH);
	data = inPortByte(RTC_CMOS_DATA);
	*dayOfMonth = RTC_BCD_TO_BIN(data);

	outPortByte(RTC_CMOS_ADDRESS, RTC_ADDRESS_DAYOFWEEK);
	data = inPortByte(RTC_CMOS_DATA);
	*dayOfWeek = RTC_BCD_TO_BIN(data);
}

char* convertDayOfWeekToString(BYTE dayOfWeek)
{
	static char* dayOfWeekString[8] = {
		"Error",
		"Sunday",
		"Monday",
		"Tuesday",
		"Wednesday",
		"Thursday",
		"Friday",
		"Saturday"
	};

	if (dayOfWeek > 7)
		return dayOfWeekString[0];
	return dayOfWeekString[dayOfWeek];
}
