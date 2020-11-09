#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTES_OF_SECTOR		512

int adjustInSectorSize(int fd, int srcSize);
void writeKernelInformation(int fd, int totalKernelSectorCount,
	int kernel32SectorCount);
int copyFile(int srcFd, int targetFd);

int main(int argc, char *argv[])
{
	int srcFd;
	int targetFd;
	int bootLoaderSectorCount;
	int kernel32SectorCount;
	int kernel64SectorCount;
	int srcSize;

	if (argc < 4)
	{
		fprintf(stderr, "[ERROR] ./ImageMaker BootLoader.bin Kernel32.bin Kernel64.bin\n");
		exit(-1);
	}

	if ((targetFd = open("Disk.img", O_RDWR | O_CREAT | O_TRUNC,
		S_IREAD | S_IWRITE)) == -1)
	{
		fprintf(stderr, "[ERROR] Disk.img open file\n");
		exit(-1);
	}

	// open boot loader and copy to Disk.img
	printf("[INFO] Copy boot loader to image file\n");
	if ((srcFd = open(argv[1], O_RDONLY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open fail\n", argv[1]);
		exit(-1);
	}
	srcSize = copyFile(srcFd, targetFd);
	close(srcFd);

	bootLoaderSectorCount = adjustInSectorSize(targetFd, srcSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
		argv[1], srcSize, bootLoaderSectorCount);

	// open kernel 32 and copy to Disk.img
	printf("[INFO] Copy protected mode kernel to image file\n");
	if ((srcFd = open(argv[2], O_RDONLY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open fail\n", argv[2]);
		exit(-1);
	}
	srcSize = copyFile(srcFd, targetFd);
	close(srcFd);

	kernel32SectorCount = adjustInSectorSize(targetFd, srcSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
		argv[2], srcSize, kernel32SectorCount);

	// open kernel 64 and copy to Disk.img
	printf("[INFO] Copy IA-32e kernel to image file\n");
	if ((srcFd = open(argv[3], O_RDONLY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open fail\n", argv[3]);
		exit(-1);
	}
	srcSize = copyFile(srcFd, targetFd);
	close(srcFd);

	kernel64SectorCount = adjustInSectorSize(targetFd, srcSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
		argv[3], srcSize, kernel64SectorCount);

	// modify kernel info
	printf("[INFO] Start to write kernel information\n");
	writeKernelInformation(targetFd, kernel32SectorCount + kernel64SectorCount,
		kernel32SectorCount);

	printf("[INFO] Image file create complete\n");
	close(targetFd);
	return 0;
}

int adjustInSectorSize(int fd, int srcSize)
{
	int i;
	int sizeToAdjust;
	char ch;

	sizeToAdjust = srcSize % BYTES_OF_SECTOR;
	ch = 0x00;
	if (sizeToAdjust)
	{
		sizeToAdjust = 512 - sizeToAdjust;
		for (i=0; i < sizeToAdjust; i++)
		{
			write(fd, &ch, 1);
		}
	}
	else
		printf("[INFO] File size is already aligned\n");
	return (srcSize + sizeToAdjust) / BYTES_OF_SECTOR;
}

void writeKernelInformation(int fd, int totalKernelSectorCount,
	int kernel32SectorCount)
{
	long pos;
	unsigned short data;

	pos = lseek(fd, 5, SEEK_SET);
	if (pos == -1)
	{
		fprintf(stderr, "lseek fail. Return value = %ld, errno = %d, %d\n",
			pos, errno, SEEK_SET);
		exit(-1);
	}
	data = (unsigned short)totalKernelSectorCount;
	write(fd, &data, 2);
	data = (unsigned short)kernel32SectorCount;
	write(fd, &data, 2);

	printf("[INFO] Total sector count except boot loader [%d]\n",
		totalKernelSectorCount);
	printf("[INFO] Total sector count of protected mode kernel [%d]\n",
		kernel32SectorCount);
}

int copyFile(int srcFd, int targetFd)
{
	int total = 0;
	int r;
	int w;
	char buffer[BYTES_OF_SECTOR];

	while ((r = read(srcFd, buffer, sizeof(buffer))) != 0)
	{
		w = write(targetFd, buffer, r);
		if (r != w)
		{
			fprintf(stderr, "[ERROR] r != w\n");
			exit(-1);
		}
		total += r;
	}
	return total;
}
