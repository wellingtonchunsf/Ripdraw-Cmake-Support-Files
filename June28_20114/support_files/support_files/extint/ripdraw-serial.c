/* ripdraw.cpp
 * 
 * supports Windows/Linux only
 * supports little-endian CPU only
 * 
 */
#if 0

#include "ripdraw.h"

typedef struct _RD_INTERFACE_SERIAL
{
#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle;
#else
    int handle;
#endif
} RD_INTERFACE_SERIAL;

#if defined(_WIN32) || defined(_WIN64)

/* ================================================================== */
/* open the serial port */
int rd_extint_open(RD_INTERFACE* rd_interface, char* port_name)
{
    HANDLE handle;
    char device_name[16];
    DCB dcb;
    COMMTIMEOUTS cto;
    DWORD DesiredAccess = GENERIC_READ | GENERIC_WRITE;
    DWORD ShareMode = FILE_SHARE_READ;
    DWORD CreationDis = OPEN_ALWAYS;
    DWORD FlagsAndAttr = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE;

    if (rd_interface == NULL)
    {
        fprintf(stderr, "interface should not NULL");
        return -1;
    }

    device_name[0] = 0;
    if (port_name[0] != '\\')
    {
        strcpy_s(device_name, sizeof(device_name) - 1, "\\\\.\\");
    }
    strcat_s(device_name, sizeof(device_name) - 1, port_name);

    handle = CreateFile(device_name, DesiredAccess, ShareMode, NULL, CreationDis, FlagsAndAttr, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "port open failed");
        return -1;
    }

    FillMemory(&dcb, sizeof(dcb), 0);
    if (!GetCommState(handle, &dcb))
    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }
    dcb.BaudRate = CBR_115200;
    dcb.Parity = 0;
    dcb.StopBits = 0;
    dcb.ByteSize = 8;
    if (!SetCommState(handle, &dcb))
    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }

    if (!GetCommTimeouts(handle, &cto))
    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }
    cto.ReadIntervalTimeout = 0;
    cto.ReadTotalTimeoutConstant = 0;
    cto.ReadTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(handle, &cto))
    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }

	rd_interface->extint = malloc(sizeof(RD_INTERFACE_SERIAL));
	if (!rd_interface->extint)
	{
		CloseHandle(handle);
		fprintf(stderr, "external interface data not allocated");
		return -1;
	}

    rd_interface->is_open = 1;
    ((RD_INTERFACE_SERIAL*)rd_interface->extint)->handle = handle;
	rd_interface->seq_no = 0;
    return 0;
}

/* ================================================================== */
/* close serial port */
int rd_extint_close(RD_INTERFACE* rd_interface)
{
	RD_INTERFACE_SERIAL* extint = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    _RD_CHECK_INTERFACE();

    rd_interface->is_open = 0;
	CloseHandle(extint->handle);
	free(extint);

    return 0;
}

/* ================================================================== */
/* write data to serial */
int rd_extint_write(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len)
{
	RD_INTERFACE_SERIAL* extint = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    int total_write = 0;
    DWORD bytes_write;
    _RD_CHECK_INTERFACE();

    do
    {
        if (WriteFile(extint->handle, data_ptr, data_len - total_write, &bytes_write, NULL) == FALSE)
        {
            return -1;
        }
        total_write += bytes_write;
        data_ptr += bytes_write;
    }
    while (total_write < data_len);
    return 0;
}

/* ================================================================== */
/* read data from serial */
int rd_extint_read(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len)
{
	RD_INTERFACE_SERIAL* extint = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    int total_read = 0;
    DWORD bytes_read;
    _RD_CHECK_INTERFACE();

    do
    {
        if (ReadFile(extint->handle, data_ptr, data_len - total_read, &bytes_read, NULL) == FALSE)
        {
            return -1;
        }
        total_read += bytes_read;
        data_ptr += bytes_read;
    }
    while (total_read < data_len);
    return 0;
}

#else

/* ================================================================== */
/* open the serial port */
int rd_extint_open(RD_INTERFACE* rd_interface, char* port_name)
{
    speed_t speed = B115200;
    int handle;
    struct termios settings;

    if (rd_interface == NULL)
    {
        fprintf(stderr, "interface should not NULL");
        return -1;
    }

    handle = open(port_name, O_RDWR | O_NOCTTY);
    if (handle < 0)
    {
        fprintf(stderr, "port open failed");
        return -1;
    }

    if (tcgetattr(handle, &settings) < 0)

    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }

    /* set input mode to raw, no echo
    * set output mode to raw */
    cfmakeraw(&settings);

    /* blocking mode */
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 10;

    settings.c_line = N_TTY;

    /* Set the baud rate for both input and output. */
    if ((cfsetispeed(&settings, speed) < 0) || (cfsetospeed(&settings, speed) < 0))
    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }

    /* set no parity, stop bits, data bits */
    settings.c_cflag &= ~PARENB;
    settings.c_cflag &= ~(CSTOPB | CRTSCTS);
    settings.c_cflag &= ~CSIZE;
    settings.c_cflag |= CS8;

    if (tcsetattr(handle, TCSANOW, &settings) < 0)
    {
        fprintf(stderr, "port initialization failed");
        return -1;
    }

    tcflush(handle, TCIOFLUSH);

	rd_interface->extint = malloc(sizeof(RD_INTERFACE_SERIAL));
	if (!rd_interface->extint)
	{
		close(handle);
		fprintf(stderr, "external interface data not allocated");
		return -1;
	}

    rd_interface->is_open = 1;
    ((RD_INTERFACE_SERIAL*)rd_interface->extint)->handle = handle;
    return 0;
}

/* ================================================================== */
/* close serial port */
int rd_extint_close(RD_INTERFACE* rd_interface)
{
	RD_INTERFACE_SERIAL* extint = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    _RD_CHECK_INTERFACE();

    rd_interface->is_open = 0;
    close(extint->handle);
	free(extint);
	
	return 0;
}

/* ================================================================== */
/* write data to serial */
int rd_extint_write(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len)
{
	RD_INTERFACE_SERIAL* extint = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    int total_write = 0;
    int bytes_write;
    _RD_CHECK_INTERFACE();

    do
    {
        bytes_write = write(extint->handle, data_ptr, data_len - total_write);
        if (bytes_write < 0)
        {
            return bytes_write;
        }
        total_write += bytes_write;
        data_ptr += bytes_write;
    }
    while (total_write < data_len);
    return 0;
}

/* ================================================================== */
/* read data from serial */
int rd_extint_read(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len)
{
	RD_INTERFACE_SERIAL* extint = (RD_INTERFACE_SERIAL*)rd_interface->extint;
    int total_read = 0;
    int bytes_read;
    _RD_CHECK_INTERFACE();

    do
    {
        bytes_read = read(extint->handle, data_ptr, data_len - total_read);
        if (bytes_read < 0)
        {
            return bytes_read;
        }
        total_read += bytes_read;
        data_ptr += bytes_read;
    }
    while (total_read < data_len);
    return 0;
}
#endif

#endif
