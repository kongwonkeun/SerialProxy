#ifndef SOCK_H
#define SOCK_H

#include <stdio.h>
#include <winsock.h>

typedef struct
{
    int fd;
    int port;
}
tcpsock_s;

int  sock_start(void);
void sock_finish(void);

int  tcp_init(tcpsock_s *sock, int port);
void tcp_cleanup(tcpsock_s *sock);
int  tcp_listen(tcpsock_s *sock);
tcpsock_s *tcp_accept(tcpsock_s *sock);
void tcp_refuse(tcpsock_s *sock);
int  tcp_connect(tcpsock_s *sock, char *host, int port);
void tcp_disconnect(tcpsock_s *sock);
int  tcp_read(tcpsock_s *sock, void *buf, size_t count);
int  tcp_write(tcpsock_s *sock, void *buf, size_t count);
int  tcp_getport(tcpsock_s *sock);

#endif /* SOCK_H */
