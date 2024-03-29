#ifndef SIO_H
#define SIO_H

#define SIO_OUT 0x01
#define SIO_IN  0x02

#define SIO_PARITY_NONE 0
#define SIO_PARITY_EVEN 1
#define SIO_PARITY_ODD  2

#include <stdio.h>
#include <windows.h>

typedef struct
{
    short port;
    long  baud;
    int   parity;
    int   stopbits;
    int   databits;
    char  serial_device[30];
}
serialinfo_s;

typedef struct
{
    HANDLE fd;
    HANDLE hComm;
    serialinfo_s info;
}
sio_s;

int  sio_init(sio_s *sio);
void sio_cleanup(sio_s *sio);

int  sio_open(sio_s *sio);
void sio_close(sio_s *sio);
int  sio_read(sio_s *sio, void *buf, size_t count);
int  sio_write(sio_s *sio, void *buf, size_t count);
int  sio_isopen(sio_s *sio);
int  sio_setinfo(sio_s *sio, serialinfo_s *info);
void sio_flush(sio_s *sio, int dir);
void sio_drain(sio_s *sio);

#endif /* SIO_H */
