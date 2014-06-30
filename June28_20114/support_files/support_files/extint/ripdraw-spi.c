/* ripdraw.cpp
 * 
 * supports Windows/Linux only
 * supports little-endian CPU only
 * 
 */
#if 1

#include "ripdraw.h"
#if defined(_WIN32) || defined(_WIN64)
#include "windows/ftd2xx.h"
#else
#include "linux/ftd2xx.h"
#endif

typedef struct _RD_INTERFACE_SERIAL
{
	FT_HANDLE handle;
} RD_INTERFACE_SERIAL;

void SPI_CSEnable(BYTE* outputBuffer, DWORD* outputLen, int enable, int loop_count)
{
	int loop;
	unsigned long i = *outputLen;

	//loop_count = loop_count;
	// one 0x80 command can keep 0.2us, do 5 times to stay in this situation for 1us
	for(loop = 0; loop < loop_count; loop++)
	{
		//Command to set directions of lower 8 pins and force value on bits set as output
		outputBuffer[i++] = '\x80';
		if (enable)
		{
			//	CS		MISO	MOSI	SCK
			//	0		0		0		0
			outputBuffer[i++] = '\x00';
		}
		else
		{
			//	CS		MISO	MOSI	SCK
			//	1		0		0		0
			outputBuffer[i++] = '\x08';
		}
		//	CS		MISO	MOSI	SCK
		//	OUT		IN		OUT		OOUT
		outputBuffer[i++] = '\x0b';
	}
	*outputLen = i;
}

int SPI_Purge(RD_INTERFACE_SERIAL* spi, int read, int write)
{
	ULONG mask = 0;
	if (read)
	{
		mask |= FT_PURGE_RX;
	}
	if (write)
	{
		mask |= FT_PURGE_TX;
	}
	FT_Purge(spi->handle, mask);
	return 0;
}

/* ================================================================== */
/* open the serial port */
int rd_extint_open(RD_INTERFACE* rd_interface, const char* port_name)
{
 	FT_STATUS ftStatus;
	FT_HANDLE handle;
	BYTE InputBuffer[512];
	DWORD dwNumInputBuffer;
	DWORD dwNumBytesRead;
	BYTE outputBuffer[512];
	DWORD outputLen;
	DWORD dwNumBytesSent;
	DWORD dwClockDivisor;

	if (rd_interface == NULL)
    {
        fprintf(stderr, "interface should not NULL");
        return -020301;
    }
	memset(rd_interface, 0, sizeof(RD_INTERFACE));

	rd_interface->is_open = 0;
	ftStatus = FT_Open(0, &handle);
	if (ftStatus != FT_OK)
	{
		fprintf(stderr, "FT open failed, error: %d\n", ftStatus);
		return -020302;
	}
	RD_DBG(0, "Successfully open FT2232H device!\n");

	//Reset USB device
	ftStatus = FT_ResetDevice(handle);

	// Purge USB receive buffer first by reading out all old data from FT2232H receive buffer
	// Get the number of bytes in the FT2232H receive buffer

	ftStatus |= FT_GetQueueStatus(handle, &dwNumInputBuffer);
	if ((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
	{
		//Read out the data from FT2232H receive buffer
		ftStatus |= FT_Read(handle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
	}

	//Set USB request transfer size
	ftStatus |= FT_SetUSBParameters(handle, 65535, 65535);
	//Disable event and error characters
	ftStatus |= FT_SetChars(handle, 0, 0, 0, 0);
	//Sets the read and write timeouts in 3 sec for the FT2232H
	ftStatus |= FT_SetTimeouts(handle, 3000, 3000);
	//Set the latency timer
	ftStatus |= FT_SetLatencyTimer(handle, 1);
	//Reset controller
	ftStatus |= FT_SetBitMode(handle, 0x0, 0x00);
	//Enable MPSSE mode
	ftStatus |= FT_SetBitMode(handle, 0x0, 0x02);
	if (ftStatus != FT_OK)
	{
		fprintf(stderr, "FT init failed, error: %d\n", ftStatus);
		return -020303;
	}
	// Wait 50ms for all the USB stuff to complete and work
	RD_SLEEP(50);

	outputLen = 0;
	//Ensure disable clock divide by 5 for 60Mhz master clock
	// 0x8A
	outputBuffer[outputLen++] = '\x8A';
	//Ensure turn off adaptive clocking
	outputBuffer[outputLen++] = '\x97';
	//disable 3 phase data clock
	outputBuffer[outputLen++] = '\x8D';
	ftStatus = FT_Write(handle, outputBuffer, outputLen, &dwNumBytesSent);

	outputLen = 0;
	SPI_CSEnable(outputBuffer, &outputLen, 0, 1);

	// The SK clock frequency can be worked out by below algorithm with divide by 5 set as off
	// SK frequency = 60MHz /((1 + [(1 +0xValueH*256) OR 0xValueL])*2)
	//Command to set clock divisor
	outputBuffer[outputLen++] = '\x86';
	// 60/((1+29)*2) = 1Mhz
	// 60/((1+14)*2) = 2Mhz
	// 60/((1+9)*2) = 3Mhz
	dwClockDivisor = 14;
	//Set 0xValueL of clock divisor
	outputBuffer[outputLen++] = (BYTE)(dwClockDivisor & '\xFF');
	//Set 0xValueH of clock divisor
	outputBuffer[outputLen++] = (BYTE)(dwClockDivisor >> 8);
	ftStatus = FT_Write(handle, outputBuffer, outputLen, &dwNumBytesSent);
	RD_DBG(1, "SPI ClockDivisor: %d\n", dwClockDivisor);

	RD_SLEEP(20);

	outputLen = 0;
	//Turn off loop back in case
	//Command to turn off loop back of TDI/TDO connection
	outputBuffer[outputLen++] = '\x85';
	ftStatus = FT_Write(handle, outputBuffer, outputLen, &dwNumBytesSent);

	RD_SLEEP(30);

	rd_interface->extint = malloc(sizeof(RD_INTERFACE_SERIAL));
	if (!rd_interface->extint)
	{
		fprintf(stderr, "external interface data not allocated");
		return -020304;
	}

    rd_interface->is_open = 1;
    ((RD_INTERFACE_SERIAL*)rd_interface->extint)->handle = handle;
	rd_interface->seq_no = 0;

	SPI_Purge(((RD_INTERFACE_SERIAL*)rd_interface->extint), 1, 1);
    return 0;
}

/* ================================================================== */
/* close serial port */
int rd_extint_close(RD_INTERFACE* rd_interface)
{
	RD_INTERFACE_SERIAL* spi = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    _RD_CHECK_INTERFACE();

	rd_interface->is_open = 0;

	FT_ResetDevice(spi->handle);
	FT_Close(spi->handle);
	free(spi);

    return 0;
}

/* ================================================================== */
/* write data to serial */
int rd_extint_write(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len)
{
	RD_INTERFACE_SERIAL* spi = (RD_INTERFACE_SERIAL*)rd_interface->extint;
	FT_STATUS ftStatus;
	BYTE outputBuffer[4096];
	DWORD outputLen;
	DWORD dwNumBytesSent;
	int i;
	 _RD_CHECK_INTERFACE();

	//SPI_Purge(spi, 1, 1);

	outputLen = 0;
	for (i = 0; i < data_len; i++)
	{
		SPI_CSEnable(outputBuffer, &outputLen, 1, 5);
		// on -ve clock edge MSB first
#if 0
		outputBuffer[outputLen++] = '\x11';
		// length
		outputBuffer[outputLen++] = 0;
		outputBuffer[outputLen++] = 0;
#else
		outputBuffer[outputLen++] = '\x13';
		// length
		outputBuffer[outputLen++] = 7;
#endif
		outputBuffer[outputLen++] = data_ptr[i];
		SPI_CSEnable(outputBuffer, &outputLen, 0, 5);

		if (outputLen > sizeof(outputBuffer) - 64)
		{
			ftStatus = FT_Write(spi->handle, outputBuffer, outputLen, &dwNumBytesSent);
			if (ftStatus != FT_OK)
			{
				fprintf(stderr, "FT write, error: %d\n", ftStatus);
				return -020501;
			}
			outputLen = 0;
		}
	}
	if (outputLen > 0)
	{
		ftStatus = FT_Write(spi->handle, outputBuffer, outputLen, &dwNumBytesSent);
		if (ftStatus != FT_OK)
		{
			fprintf(stderr, "FT write, error: %d\n", ftStatus);
			return -020502;
		}
	}

	RD_SLEEP(2);

	return data_len;
}

/* ================================================================== */
/* read data from serial */
int rd_extint_read(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len)
{
	RD_INTERFACE_SERIAL* spi = (RD_INTERFACE_SERIAL*)rd_interface->extint;
#define inputBufferMax 512
	FT_STATUS ftStatus;
	BYTE outputBuffer[4096];
	DWORD outputLen;
	DWORD dwNumBytesSent;
	BYTE InputBuffer[inputBufferMax];
	DWORD dwNumInputBuffer;
	DWORD dwNumBytesRead = 0;
	int i;

	if (data_len > inputBufferMax)
	{
		data_len = inputBufferMax;
	}

	//SPI_Purge(spi, 1, 1);

#if 1
	outputLen = 0;
	for (i = 0; i < data_len; i++)
	{
		SPI_CSEnable(outputBuffer, &outputLen, 1, 5);
		// on +ve clock edge MSB first
#if 0
		outputBuffer[outputLen++] = '\x20';
		// length
		outputBuffer[outputLen++] = 0;
		outputBuffer[outputLen++] = 0;
#else
		outputBuffer[outputLen++] = '\x22';
		// length
		outputBuffer[outputLen++] = 7;
#endif
		SPI_CSEnable(outputBuffer, &outputLen, 0, 5);

		if (outputLen > sizeof(outputBuffer) - 64)
		{
			ftStatus = FT_Write(spi->handle, outputBuffer, outputLen, &dwNumBytesSent);
			if (ftStatus != FT_OK)
			{
				fprintf(stderr, "FT write, error: %d\n", ftStatus);
				return -020601;
			}
			outputLen = 0;
		}
	}
	if (outputLen > 0)
	{
		ftStatus = FT_Write(spi->handle, outputBuffer, outputLen, &dwNumBytesSent);
		if (ftStatus != FT_OK)
		{
			fprintf(stderr, "FT write, error: %d\n", ftStatus);
			return -020602;
		}
	}

	memset(InputBuffer, 0, inputBufferMax);
	dwNumInputBuffer = data_len;
	ftStatus = FT_Read(spi->handle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
	if (ftStatus != FT_OK)
	{
		fprintf(stderr, "FT read, error: %d\n", ftStatus);
		return -020603;
	}

#else
	outputLen = 0;
	SPI_CSEnable(outputBuffer, &outputLen, 1);
	// Clock Data Bytes In on -ve clock edge MSB first
	outputBuffer[outputLen++] = '\x24';
	// A length of 0x0000 will do 1 byte and a length of 0xffff will do 65536 bytes
	outputBuffer[outputLen++] = len - 1;
	outputBuffer[outputLen++] = 0;
	SPI_CSEnable(outputBuffer, &outputLen, 0);
	ftStatus = FT_Write(spi->handle, outputBuffer, outputLen, &dwNumBytesSent);

	dwNumInputBuffer = len;
	ftStatus = FT_Read(spi->handle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
#endif

	for (i = 0; i < (int)dwNumBytesRead; i++)
	{
		data_ptr[i] = InputBuffer[i];
	}

	return dwNumBytesRead;
}

#endif
