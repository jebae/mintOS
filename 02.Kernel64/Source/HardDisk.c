#include "HardDisk.h"
#include "AssemblyUtility.h"
#include "Console.h"

static HDDMANAGER gHDDManager;

BOOL initHDD(void)
{
	initMutex(&gHDDManager.mutex);
	gHDDManager.primaryInterruptOccur = FALSE;
	gHDDManager.secondaryInterruptOccur = FALSE;
	outPortByte(HDD_PORT_PRIMARY_BASE + HDD_PORT_INDEX_DIGITAL_OUTPUT, 0);
	outPortByte(HDD_PORT_SECONDARY_BASE + HDD_PORT_INDEX_DIGITAL_OUTPUT, 0);

	if (readHDDInformation(TRUE, TRUE, &gHDDManager.HDDInformation) == FALSE)
	{
		gHDDManager.HDDDetected = FALSE;
		gHDDManager.canWrite = FALSE;
		return FALSE;
	}
	gHDDManager.HDDDetected = TRUE;
	if (memcmp(gHDDManager.HDDInformation.modelNumber, "QEMU", 4) == 0)
		gHDDManager.canWrite = TRUE;
	else
		gHDDManager.canWrite = FALSE;
	return TRUE;
}

static BYTE readHDDStatus(BOOL isPrimary)
{
	if (isPrimary == TRUE)
		return inPortByte(HDD_PORT_PRIMARY_BASE + HDD_PORT_INDEX_STATUS);
	return inPortByte(HDD_PORT_SECONDARY_BASE + HDD_PORT_INDEX_STATUS);
}

static BOOL waitForHDDBusy(BOOL isPrimary)
{
	QWORD tickCount;

	tickCount = getTickCount();
	while (getTickCount() - tickCount < HDD_WAIT_TIME)
	{
		if ((readHDDStatus(isPrimary) & HDD_STATUS_BUSY) != HDD_STATUS_BUSY)
			return TRUE;
		sleep(1);
	}
	return FALSE;
}

static BOOL waitForHDDReady(BOOL isPrimary)
{
	QWORD tickCount;

	tickCount = getTickCount();
	while (getTickCount() - tickCount < HDD_WAIT_TIME)
	{
		if ((readHDDStatus(isPrimary) & HDD_STATUS_READY) == HDD_STATUS_READY)
			return TRUE;
		sleep(1);
	}
	return FALSE;
}

void setHDDInterruptFlag(BOOL isPrimary, BOOL flag)
{
	if (isPrimary)
		gHDDManager.primaryInterruptOccur = flag;
	else
		gHDDManager.secondaryInterruptOccur = flag;
}

static BOOL waitForHDDInterrupt(BOOL isPrimary)
{
	QWORD tickCount;

	tickCount = getTickCount();
	while (getTickCount() - tickCount < HDD_WAIT_TIME)
	{
		if (isPrimary && gHDDManager.primaryInterruptOccur)
			return TRUE;
		if (!isPrimary && gHDDManager.secondaryInterruptOccur)
			return TRUE;
		sleep(1);
	}
	return FALSE;
}

BOOL readHDDInformation(BOOL isPrimary, BOOL isMaster, HDDINFORMATION* information)
{
	int i;
	WORD portBase;
	BYTE driveFlag, status;
	BOOL waitRes;

	portBase = (isPrimary)
		? HDD_PORT_PRIMARY_BASE
		: HDD_PORT_SECONDARY_BASE;
	lock(&gHDDManager.mutex);

	if (!waitForHDDBusy(isPrimary))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	driveFlag = (isMaster)
		? HDD_DRIVE_AND_HEAD_LBA
		: (HDD_DRIVE_AND_HEAD_LBA | HDD_DRIVE_AND_HEAD_SLAVE);

	outPortByte(portBase + HDD_PORT_INDEX_DRIVE_AND_HEAD, driveFlag);

	if (!waitForHDDReady(isPrimary))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	setHDDInterruptFlag(isPrimary, FALSE);
	outPortByte(portBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTITY);

	waitRes = waitForHDDInterrupt(isPrimary);
	status = readHDDStatus(isPrimary);

	if (!waitRes || ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	for (i=0; i < 512 / 2; i++)
	{
		((WORD*)information)[i] = inPortWord(portBase + HDD_PORT_INDEX_DATA);
	}
	swapByteInWord(information->modelNumber, sizeof(information->modelNumber) / 2);
	swapByteInWord(information->serialNumber, sizeof(information->serialNumber) / 2);

	unlock(&gHDDManager.mutex);
	return TRUE;
}

static void swapByteInWord(WORD* data, int wordCount)
{
	int i;

	for (i=0; i < wordCount; i++)
	{
		data[i] = (data[i] >> 8) | (data[i] << 8);
	}
}

int readHDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf)
{
	WORD portBase;
	BYTE driveFlag, status;
	BOOL waitRes;
	int i, j;
	long readCount = 0;

	if (!gHDDManager.HDDDetected ||
		sectorCount <= 0 || sectorCount > 256 ||
		LBA + sectorCount >= gHDDManager.HDDInformation.totalSectors)
		return 0;

	portBase = (isPrimary)
		? HDD_PORT_PRIMARY_BASE
		: HDD_PORT_SECONDARY_BASE;
	lock(&gHDDManager.mutex);

	if (!waitForHDDBusy(isPrimary))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	outPortByte(portBase + HDD_PORT_INDEX_SECTOR_COUNT, sectorCount);
	outPortByte(portBase + HDD_PORT_INDEX_SECTOR_NUMBER, LBA);
	outPortByte(portBase + HDD_PORT_INDEX_CYLINDER_LSB, LBA >> 8);
	outPortByte(portBase + HDD_PORT_INDEX_CYLINDER_MSB, LBA >> 16);

	driveFlag = (isMaster)
		? HDD_DRIVE_AND_HEAD_LBA
		: (HDD_DRIVE_AND_HEAD_LBA | HDD_DRIVE_AND_HEAD_SLAVE);

	outPortByte(portBase + HDD_PORT_INDEX_DRIVE_AND_HEAD, driveFlag | ((LBA >> 24) & 0x0F));

	if (!waitForHDDReady(isPrimary))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	setHDDInterruptFlag(isPrimary, FALSE);
	outPortByte(portBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

	for (i=0; i < sectorCount; i++)
	{
		status = readHDDStatus(isPrimary);
		if ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)
		{
			printf("HDD read error occur\n");
			unlock(&gHDDManager.mutex);
			return i;
		}

		if ((status & HDD_STATUS_DATA_REQUEST) != HDD_STATUS_DATA_REQUEST)
		{
			waitRes = waitForHDDInterrupt(isPrimary);
			setHDDInterruptFlag(isPrimary, FALSE);
			if (!waitRes)
			{
				printf("HDD read interrupt not occur\n");
				unlock(&gHDDManager.mutex);
				return FALSE;
			}
		}

		for (j=0 ; j < 512 / 2; j++)
		{
			((WORD*)buf)[readCount++] = inPortWord(portBase + HDD_PORT_INDEX_DATA);
		}
	}
	unlock(&gHDDManager.mutex);
	return i;
}

int writeHDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf)
{
	WORD portBase;
	BYTE driveFlag, status;
	BOOL waitRes;
	int i, j;
	long readCount = 0;

	if (!gHDDManager.HDDDetected ||
		sectorCount <= 0 || sectorCount > 256 ||
		LBA + sectorCount >= gHDDManager.HDDInformation.totalSectors)
		return 0;

	portBase = (isPrimary)
		? HDD_PORT_PRIMARY_BASE
		: HDD_PORT_SECONDARY_BASE;
	lock(&gHDDManager.mutex);

	if (!waitForHDDBusy(isPrimary))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	outPortByte(portBase + HDD_PORT_INDEX_SECTOR_COUNT, sectorCount);
	outPortByte(portBase + HDD_PORT_INDEX_SECTOR_NUMBER, LBA);
	outPortByte(portBase + HDD_PORT_INDEX_CYLINDER_LSB, LBA >> 8);
	outPortByte(portBase + HDD_PORT_INDEX_CYLINDER_MSB, LBA >> 16);

	driveFlag = (isMaster)
		? HDD_DRIVE_AND_HEAD_LBA
		: (HDD_DRIVE_AND_HEAD_LBA | HDD_DRIVE_AND_HEAD_SLAVE);

	outPortByte(portBase + HDD_PORT_INDEX_DRIVE_AND_HEAD, driveFlag | ((LBA >> 24) & 0x0F));

	if (!waitForHDDReady(isPrimary))
	{
		unlock(&gHDDManager.mutex);
		return FALSE;
	}

	setHDDInterruptFlag(isPrimary, FALSE);
	outPortByte(portBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

	while (1)
	{
		status = readHDDStatus(isPrimary);
		if ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)
		{
			unlock(&gHDDManager.mutex);
			return 0;
		}
		if ((status & HDD_STATUS_DATA_REQUEST) == HDD_STATUS_DATA_REQUEST)
			break;
		sleep(1);
	}

	for (i=0; i < sectorCount; i++)
	{
		for (j=0; j < 512 / 2; j++)
		{
			outPortWord(portBase + HDD_PORT_INDEX_DATA, ((WORD*)buf)[readCount++]);
		}
		status = readHDDStatus(isPrimary);
		if ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)
		{
			unlock(&gHDDManager.mutex);
			return i;
		}
		if ((status & HDD_STATUS_DATA_REQUEST) != HDD_STATUS_DATA_REQUEST)
		{
			waitRes = waitForHDDInterrupt(isPrimary);
			setHDDInterruptFlag(isPrimary, FALSE);
			if (!waitRes)
			{
				printf("HDD write interrupt not occur\n");
				unlock(&gHDDManager.mutex);
				return FALSE;
			}
		}
	}
	unlock(&gHDDManager.mutex);
	return i;
}
