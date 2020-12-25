#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DWORD unsigned int
#define BYTE unsigned char
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define SERIAL_FIFO_MAX_SIZE	16

int main(int argc, char** argv)
{
	char fileName[256];
	char buf[SERIAL_FIFO_MAX_SIZE];
	struct sockaddr_in socketAddr;
	int sock;
	BYTE ack;
	DWORD dataLength;
	DWORD sentSize;
	DWORD temp;
	FILE* fp;

	if (argc < 2)
	{
		fprintf(stderr, "input file name: ");
		gets(fileName);
	}
	else
	{
		strcpy(fileName, argv[1]);
	}

	fp = fopen(fileName, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "%s file open error\n", fileName);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	dataLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "file name: %s, data length: %d byte\n", fileName, dataLength);

	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(4444);
	socketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sock, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) == -1)
	{
		fprintf(stderr, "socket connect error\n");
		return 0;
	}
	else
	{
		fprintf(stderr, "socket connect success\n");
	}

	if (send(sock, &dataLength, 4, 0) != 4)
	{
		fprintf(stderr, "fail to send data length\n");
		return 0;
	}
	else
	{
		fprintf(stderr, "data length send success\n");
	}

	if (recv(sock, &ack, 1, 0) != 1)
	{
		fprintf(stderr, "ACK receive error (data length)\n");
		return 0;
	}

	fprintf(stderr, "data transfer...");
	sentSize = 0;
	while (sentSize < dataLength)
	{
		temp = MIN(dataLength - sentSize, SERIAL_FIFO_MAX_SIZE);
		sentSize += temp;
		if (fread(buf, 1, temp, fp) != temp)
		{
			fprintf(stderr, "file read error\n");
			return 0;
		}

		if (send(sock, buf, temp, 0) != temp)
		{
			fprintf(stderr, "socket send error\n");
			return 0;
		}

		if (recv(sock, &ack, 1, 0) != 1)
		{
			fprintf(stderr, "ACK receive error (data)\n");
			return 0;
		}
		fprintf(stderr, "#");
	}

	fclose(fp);
	close(sock);

	fprintf(stderr, "send complete. [%d] byte\n", sentSize);
	return 0;
}
