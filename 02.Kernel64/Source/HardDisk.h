#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "Synchronization.h"

#define HDD_PORT_PRIMARY_BASE												0x1F0
#define HDD_PORT_SECONDARY_BASE											0x170

#define HDD_PORT_INDEX_DATA													0x00
#define HDD_PORT_INDEX_SECTOR_COUNT									0x02
#define HDD_PORT_INDEX_SECTOR_NUMBER								0x03
#define HDD_PORT_INDEX_CYLINDER_LSB									0x04
#define HDD_PORT_INDEX_CYLINDER_MSB									0x05
#define HDD_PORT_INDEX_DRIVE_AND_HEAD								0x06
#define HDD_PORT_INDEX_STATUS												0x07
#define HDD_PORT_INDEX_COMMAND											0x07
#define HDD_PORT_INDEX_DIGITAL_OUTPUT								0x206

#define HDD_COMMAND_READ														0x20
#define HDD_COMMAND_WRITE														0x30
#define HDD_COMMAND_IDENTITY												0xEC

#define HDD_STATUS_ERROR														0x01
#define HDD_STATUS_INDEX														0x02
#define HDD_STATUS_CORRECTED_DATA										0x04
#define HDD_STATUS_DATA_REQUEST											0x08
#define HDD_STATUS_SEEK_COMPLETE										0x10
#define HDD_STATUS_WRITE_FAULT											0x20
#define HDD_STATUS_READY														0x40
#define HDD_STATUS_BUSY															0x80

#define HDD_DRIVE_AND_HEAD_LBA											0xE0
#define HDD_DRIVE_AND_HEAD_SLAVE										0x10

#define HDD_DIGITAL_OUTPUT_RESET										0x04
#define HDD_DIGITAL_OUTPUT_DISABLE_INTERRUPT				0x01

#define HDD_WAIT_TIME																500
#define HDD_MAX_BULK_SECTOR_COUNT										256

#pragma pack(push, 1)

typedef struct HDDInformationStruct
{
	WORD config;
	WORD numberOfCylinder;
	WORD reserved1;

	WORD numberOfHead;
	WORD unformattedBytesPerTrack;
	WORD unformattedBytesPerSector;

	WORD numberOfSectorPerCylinder;
	WORD interSectorGap;
	WORD bytesInPhaseLock;
	WORD numberOfVendorUniqueStatusWord;

	WORD serialNumber[10];
	WORD controllerType;
	WORD bufferSize;
	WORD numberOfECCBytes;
	WORD firmwareRevision[4];

	WORD modelNumber[20];
	WORD reserved2[13];

	DWORD totalSectors;
	WORD reserved3[196];
} HDDINFORMATION;

#pragma pack(pop)

typedef struct HDDManagerStruct
{
	BOOL HDDDetected;
	BOOL canWrite;

	volatile BOOL primaryInterruptOccur;
	volatile BOOL secondaryInterruptOccur;
	MUTEX mutex;

	HDDINFORMATION HDDInformation;
} HDDMANAGER;

BOOL initHDD(void);
BOOL readHDDInformation(BOOL isPrimary, BOOL isMaster, HDDINFORMATION* information);
int readHDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);
int writeHDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);
void setHDDInterruptFlag(BOOL isPrimary, BOOL flag);
static void swapByteInWord(WORD* data, int wordCount);
static BYTE readHDDStatus(BOOL isPrimary);
static BOOL isHDDBusy(BOOL isPrimary);
static BOOL isHDDReady(BOOL isPrimary);
static BOOL waitForHDDBusy(BOOL isPrimary);
static BOOL waitForHDDReady(BOOL isPrimary);
static BOOL waitForHDDInterrupt(BOOL isPrimary);

#endif
