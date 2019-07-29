#ifndef PIPE_H
#define PIPE_H

#include "sio.h"
#include "sock.h"
#include "thread.h"

typedef struct
{
    sio_s sio;
    tcpsock_s sock;
    thr_mutex_t *mutex;
    int timeout;
    int net_protocol;
}
pipe_s;

int  pipe_init(pipe_s *pipe, int sock_port);
void pipe_cleanup(pipe_s *pipe);
void pipe_destroy(void *data);

#endif /* PIPE_H */
