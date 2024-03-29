#include "Utility.h"
#include "AssemblyUtility.h"

volatile QWORD gTickCount = 0;

void memset(void* dest, BYTE data, int size)
{
	int i, remainedOffset;
	QWORD buf;

	buf = 0;
	for (i=0; i < 8; i++)
	{
		buf = (buf << 8) | data;
	}
	for (i=0; i < (size / 8); i++)
	{
		((QWORD*)dest)[i] = buf;
	}
	remainedOffset = i * 8;
	for (i=0; i < (size % 8); i++)
	{
		((char*)dest)[remainedOffset++] = data;
	}
}

int memcpy(void* dest, const void* src, int size)
{
	int i, remainedOffset;

	for (i=0; i < (size / 8); i++)
	{
		((QWORD*)dest)[i] = ((QWORD*)src)[i];
	}
	remainedOffset = i * 8;
	for (i=0; i < (size % 8); i++)
	{
		((char*)dest)[remainedOffset] = ((char*)src)[remainedOffset];
		remainedOffset++;
	}
	return size;
}

int memcmp(const void* a, const void* b, int size)
{
	int i, remainedOffset;
	QWORD diff;

	for (i=0; i < (size / 8); i++)
	{
		diff = ((QWORD*)a)[i] - ((QWORD*)b)[i];
		if (diff != 0)
		{
			for (i=0; i < 8; i++)
			{
				if (((diff >> (8 * i)) & 0xFF) != 0)
					return (diff >> (8 * i)) & 0xFF;
			}
		}
	}
	remainedOffset = i * 8;
	for (i=0; i < (size % 8); i++)
	{
		if (((char*)a)[remainedOffset] != ((char*)b)[remainedOffset])
			return ((char*)a)[remainedOffset] - ((char*)b)[remainedOffset];
		remainedOffset++;
	}
	return 0;
}

BOOL setInterruptFlag(BOOL enable)
{
	QWORD RFLAGS;

	RFLAGS = readRFLAGS();
	if (enable)
		enableInterrupt();
	else
		disableInterrupt();

	if (RFLAGS & 0x0200) // check if prev interrupt exist
		return TRUE;
	return FALSE;
}

int strlen(const char* buf)
{
	int i = 0;

	while (buf[i] != '\0')
		i++;
	return i;
}

static int gTotalRAMMBSize = 0;

void checkTotalRAMSize(void)
{
	DWORD* cur;
	DWORD prevValue;

	cur = (DWORD*)0x4000000;
	while (1)
	{
		prevValue = *cur;
		*cur = 0x12345678;
		if (*cur != 0x12345678)
			break;
		*cur = prevValue;
		cur += 0x100000;
	}
	gTotalRAMMBSize = (QWORD)cur / 0x100000;
}

QWORD getTotalRAMSize(void)
{
	return gTotalRAMMBSize;
}

long atoi(const char* buf, int radix)
{
	long res;

	switch (radix)
	{
	case 16:
		res = hexStringToQword(buf);
		break;
	case 10:
	default:
		res = decimalStringToLong(buf);
		break;
	}
	return res;
}

QWORD hexStringToQword(const char* buf)
{
	QWORD res = 0;

	for (int i=0; buf[i] != '\0'; i++)
	{
		res *= 16;
		if ('A' <= buf[i] && buf[i] <= 'F')
			res += 10 + (buf[i] - 'A');
		else if ('a' <= buf[i] && buf[i] <= 'f')
			res += 10 + (buf[i] - 'a');
		else
			res += buf[i] - '0';
	}
	return res;
}

long decimalStringToLong(const char* buf)
{
	long res = 0;
	int i;

	if (buf[0] == '-')
		i = 1;
	else
		i = 0;
	for (; buf[i] != '\0'; i++)
	{
		res *= 10;
		res += buf[i] - '0';
	}
	return buf[0] == '-' ? -res : res;
}

int itoa(long value, char* buf, int radix)
{
	int res;

	switch (radix)
	{
	case 16:
		res = hexToString(value, buf);
		break;
	case 10:
	default:
		res = decimalToString(value, buf);
		break;
	}
	return res;
}

int hexToString(QWORD value, char* buf)
{
	QWORD i;
	QWORD cur;

	if (value == 0)
	{
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}
	for (i=0; value > 0; i++)
	{
		cur = value % 16;
		if (cur >= 10)
			buf[i] = 'A' + (cur - 10);
		else
			buf[i] = '0' + cur;
		value /= 16;
	}
	buf[i] = '\0';
	reverseString(buf);
	return i;
}

int decimalToString(long value, char* buf)
{
	long i;

	if (value == 0)
	{
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}
	if (value < 0)
	{
		i = 1;
		buf[0] = '-';
		value = -value;
	}
	else
	{
		i = 0;
	}
	for (; value > 0; i++)
	{
		buf[i] = '0' + value % 10;
		value /= 10;
	}
	buf[i] = '\0';
	if (buf[0] == '-')
		reverseString(&buf[1]);
	else
		reverseString(buf);
	return i;
}

void reverseString(char* buf)
{
	int len;
	int until;
	int i;
	char temp;

	len = strlen(buf);
	until = len / 2;
	for (i=0; i < until; i++)
	{
		temp = buf[i];
		buf[i] = buf[len - 1 - i];
		buf[len - 1 - i] = temp;
	}
}

int sprintf(char* buf, const char* formatString, ...)
{
	va_list ap;
	int res;

	va_start(ap, formatString);
	res = vsprintf(buf, formatString, ap);
	va_end(ap);
	return res;
}

int vsprintf(char* buf, const char* formatString, va_list ap)
{
	QWORD i, k;
	int idx = 0;
	int formatLen, copyLen;
	char* copy;
	QWORD value;
	double doubleValue;

	formatLen = strlen(formatString);
	for (i=0; i < formatLen; i++)
	{
		if (formatString[i] == '%')
		{
			i++;
			switch (formatString[i])
			{
			case 's':
				copy = (char*)va_arg(ap, char*);
				copyLen = strlen(copy);
				memcpy(buf + idx, copy, copyLen);
				idx += copyLen;
				break;
			case 'c':
				buf[idx++] = (int)va_arg(ap, int);
				break;
			case 'd':
			case 'i':
				value = (QWORD)va_arg(ap, QWORD);
				idx += itoa((long)value, buf + idx, 10);
				break;
			case 'x':
			case 'X':
				value = (QWORD)va_arg(ap, DWORD) & 0xFFFFFFFF;
				idx += itoa((long)value, buf + idx, 16);
				break;
			case 'q':
			case 'Q':
			case 'p':
				value = (QWORD)va_arg(ap, QWORD);
				idx += itoa((long)value, buf + idx, 16);
				break;
			case 'f':
				doubleValue = (double)va_arg(ap, double);
				doubleValue += 0.005;
				buf[idx] = '0' + (QWORD)(doubleValue * 100) % 10;
				buf[idx + 1] = '0' + (QWORD)(doubleValue * 10) % 10;
				buf[idx + 2] = '.';
				k = 0;
				do {
					buf[idx + 3 + k++] = '0' + (QWORD)doubleValue % 10;
					doubleValue /= 10;
				} while ((QWORD)doubleValue > 0);
				buf[idx + 3 + k] = '\0';
				reverseString(buf + idx);
				idx += 3 + k;
				break;
			default:
				buf[idx++] = formatString[i];
				break;
			}
		}
		else
		{
			buf[idx++] = formatString[i];
		}
	}
	buf[idx] = '\0';
	return idx;
}

QWORD getTickCount(void)
{
	return gTickCount;
}

void sleep(QWORD millisecond)
{
	QWORD lastTickCount;

	lastTickCount = gTickCount;
	while (gTickCount - lastTickCount <= millisecond)
		schedule();
}
