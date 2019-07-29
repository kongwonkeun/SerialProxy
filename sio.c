/**/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include "sio.h"

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int sio_init(sio_s *sio)
{
    sio->fd = 0;
    sio->info.port = 1;
    sio->info.baud = 9600;
    sio->info.parity = SIO_PARITY_NONE;
    sio->info.stopbits = 1;
    sio->info.databits = 8;
    sprintf( sio->info.serial_device, "/dev/ttyS");
    return 0;
}

void sio_cleanup(sio_s *sio)
{
    if (sio_isopen(sio)) {
        sio_flush(sio, SIO_IN | SIO_OUT);
        sio_close(sio);
    }
}

int sio_open(sio_s *sio)
{
    char filename[30];
    HANDLE fd;
    HANDLE hComm;
    DCB dcb;
    COMMTIMEOUTS cto = { MAXDWORD, 0, 0, 4, 4 }; 

    if (sio_isopen(sio))
        return -1;

    sprintf(filename, "\\\\.\\COM%d", sio->info.port); 
    hComm = fd = CreateFileA( filename , GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (fd == INVALID_HANDLE_VALUE) {
        return -1;
    }
    if (!SetCommTimeouts(hComm, &cto)) {
        fprintf(stderr, "SetCommState failed.");
    }
    dcb.DCBlength = sizeof(dcb);
    memset(&dcb, sizeof(dcb), 0);

    if (!GetCommState(hComm, &dcb)) {
        fprintf(stderr, "GetCommState failed.");
    }
    dcb.BaudRate = sio->info.baud;
    dcb.fBinary = TRUE;
    dcb.fParity = (sio->info.parity == SIO_PARITY_NONE) ? FALSE : TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    dcb.ByteSize = sio->info.databits;

    switch (sio->info.parity) {
        case SIO_PARITY_NONE: dcb.Parity = 0; break;
        case SIO_PARITY_ODD:  dcb.Parity = 1; break;
        case SIO_PARITY_EVEN: dcb.Parity = 2; break;
        default: CloseHandle(fd); return -1;
    }
    switch (sio->info.stopbits) {
        case 1: dcb.StopBits = 0; break;
        case 2: dcb.StopBits = 2; break;
        default: CloseHandle(fd); return -1;
    }
    if (!SetCommState(hComm, &dcb)) {
        fprintf(stderr, "SetCommState failed.");
    }
    sio->fd = fd;
    sio->hComm = hComm;
    return 0;
}

void sio_close(sio_s *sio)
{
    CloseHandle(sio->fd);
    sio->fd = 0;
    sio->hComm = INVALID_HANDLE_VALUE;
}

int sio_read(sio_s *sio, void *buf, size_t count)
{
    int res;
    DWORD error;

    if (!ReadFile(sio->hComm, buf, (DWORD)count, &res, NULL)) {
        error = GetLastError();
        fprintf(stderr, "read failed. system error %d", error);
        return -1;
    }
    return res;
}

int sio_write(sio_s *sio, void *buf, size_t count)
{
    int res;
    DWORD error;

    if (!WriteFile(sio->hComm, buf, (DWORD)count, &res, NULL)) {
        error = GetLastError();
        fprintf(stderr, "write failed. system error %d", error);
        return -1;
    }
    return res;
}

int sio_isopen(sio_s *sio)
{
    return (sio->fd != 0);
}

int sio_setinfo(sio_s *sio, serialinfo_s *info)
{
    sio->info = *info;
    return 0;
}

void sio_flush(sio_s *sio, int dir)
{
    if (sio_isopen(sio)) {
        DWORD flags = 0;
        if (sio_isopen(sio)) {
            if (dir & SIO_IN)  flags |= PURGE_RXCLEAR | PURGE_RXABORT;
            if (dir & SIO_OUT) flags |= PURGE_TXCLEAR | PURGE_TXABORT;
        }
        PurgeComm(sio->hComm, flags);
    }
}

void sio_drain(sio_s *sio)
{
    if (sio_isopen(sio))
        FlushFileBuffers(sio->hComm);
}

/**/
