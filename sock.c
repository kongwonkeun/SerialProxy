/**/

#include <stdlib.h>
#include <sys/types.h>

#include "sock.h"

#include <winsock.h>

#define ioctl ioctlsocket

int sock_start(void)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(1, 1);
    return WSAStartup(wVersionRequested, &wsaData);
}

void sock_finish(void)
{
    WSACleanup();
}

int tcp_init(tcpsock_s *sock, int port)
{
    int optval = 1;
    
    struct sockaddr_in name;
    sock->fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (sock->fd < 0)
        return -1;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval))) {
        tcp_cleanup(sock);
        return -1;
    }
    if (ioctl(sock->fd, FIONBIO, &optval)) {
        tcp_cleanup(sock);
        return -1;
    }
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock->fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
        tcp_cleanup(sock);
        return -1;
    }
    sock->port = port;
    return 0;
}

void tcp_cleanup(tcpsock_s *sock)
{
    closesocket(sock->fd);
}

int tcp_listen(tcpsock_s *sock)
{
    return listen(sock->fd, 0);
}

tcpsock_s *tcp_accept(tcpsock_s *sock)
{
    struct sockaddr addr;
    int addrlen;
    int fd;
    tcpsock_s *newsock;
    
    addrlen = sizeof(addr);
    fd = (int)accept((SOCKET)sock->fd, &addr, &addrlen);
    if (fd < 0)
        return NULL;
    newsock = malloc(sizeof(tcpsock_s));
    if (newsock)
        newsock->fd = fd;
    return newsock;
}

void tcp_refuse(tcpsock_s *sock)
{
    tcpsock_s *temp = tcp_accept(sock);

    if (temp) {
        tcp_disconnect(temp);
        tcp_cleanup(temp);
        free(temp);
    }
}

int tcp_connect(tcpsock_s *sock, char *host, int port)
{
    struct sockaddr_in name;
    struct hostent *hostinfo;
    unsigned int addr;

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    addr = inet_addr(host);
    hostinfo = gethostbyaddr((char *)&addr, 4, PF_INET);
    if (hostinfo == NULL)
        return -1;
    name.sin_addr = *(struct in_addr *)hostinfo->h_addr;
    return connect(sock->fd, (struct sockaddr *)&name, sizeof(name));
}

void tcp_disconnect(tcpsock_s *sock)
{
    shutdown(sock->fd, 2);
}

int tcp_read(tcpsock_s *sock, void *buf, size_t count)
{
    return recv((SOCKET)sock->fd, buf, (int)count, 0);
}

int tcp_write(tcpsock_s *sock, void *buf, size_t count)
{
    return send(sock->fd, buf, (int)count, 0);
}

int tcp_getport(tcpsock_s *sock)
{
    return sock->port;
}

/**/
