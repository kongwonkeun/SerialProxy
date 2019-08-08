/**/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <io.h>

#include "sio.h"
#include "sock.h"
#include "http.h"
#include "websocket.h"
#include "pipe.h"
#include "thread.h"
#include "vlist.h"
#include "cfglib.h"
#include "config.h"
#include "error.h"

int readcfg(void);
void cleanup(void);
int waitclients(void);
thr_startfunc_t serve_pipe(void *data);
char cfgfile[] = "serialproxy.cfg";

cfg_s cfg;
vlist_s pipes;

int main(int argc, char **argv)
{
    if (sock_start())
        return -1;

    vlist_init(&pipes, pipe_destroy);
    cfg_init(&cfg, 0);
    atexit(cleanup);
    readcfg();
    waitclients();
    return 0;
}

void cleanup(void)
{
    cfg_cleanup(&cfg);
    vlist_cleanup(&pipes);
    sock_finish();
}

int readcfg(void)
{
    char ports[BUFSIZ], *p;
    int  port;
    pipe_s *pipe;
    cfg_s local;
    serialinfo_s sinfo;
    char *parity;
    
    cfg_fromfile(&cfg, cfgfile); // read the global config settings

    if (cfg_readbuf(cfgfile, "comm_ports", ports, sizeof(ports)) == NULL) // read com port list
        errend("couldn't find 'comm_ports' entry in config file '%s'", cfgfile);

    vlist_clear(&pipes);
    p = strtok(ports, ",");

    while (p) {
        if (sscanf(p, "%d", &port) > 0) {
            pipe = malloc(sizeof(pipe_s));
            if (pipe == NULL)
                perrend("malloc(pipe_s)");

            cfg_init(&local, port);
            cfg_assign(&local, &cfg);
            local.ints[CFG_IPORT].val = port; // set the com port
            cfg_fromfile(&local, cfgfile); // load this pipe's config

            if (pipe_init(pipe, local.ints[CFG_INETPORT].val))
                perrend("pipe_init");

            if (pipe != NULL) pipe->timeout = local.ints[CFG_ITIMEOUT].val;

            sinfo.port = port;
            sinfo.baud = local.ints[CFG_IBAUD].val;
            sinfo.stopbits = local.ints[CFG_ISTOP].val;
            sinfo.databits = local.ints[CFG_IDATA].val;
            memcpy(
                sinfo.serial_device,
                local.strs[CFG_SERIAL_DEVICE].val,
                strlen(local.strs[CFG_SERIAL_DEVICE].val)
            );

            if (pipe != NULL) pipe->net_protocol = local.ints[CFG_INETPROTOCOL].val;
            parity = local.strs[CFG_SPARITY].val;

            if (strcmp(parity, "none") == 0)
                sinfo.parity = SIO_PARITY_NONE;
            else if (strcmp(parity, "even") == 0)
                sinfo.parity = SIO_PARITY_EVEN;
            else if (strcmp(parity, "odd") == 0) 
                sinfo.parity = SIO_PARITY_ODD;
            else
                errend("unknown parity string '%s'", parity);

            if (sio_setinfo(&pipe->sio, &sinfo))
                errend("unable to configure comm port %d", port);
            
            // finally add the pipe to the pipes list
            vlist_add(&pipes, pipes.tail, pipe);
            cfg_cleanup(&local);
        }
        p = strtok(NULL, ",");
    }
    cfg_cleanup(&local);
    return 0;
}

int waitclients(void)
{
    int fd_max;
    fd_set fds;
    vlist_i *it;
    pipe_s *pit, *newpipe;
    tcpsock_s *newsock;
    thr_t thread;
    
    fd_max = 0;
    FD_ZERO(&fds);

    // set all sockets to listen
    for (it = pipes.head; it; it = it->next) {
        pit = (pipe_s *)it->data;
        if (tcp_listen(&pit->sock))
            perror("waitclients() - tcp_listen()");
    }

    printf("serial proxy waits a client ...\n");

    while (1) {
        for (it = pipes.head; it; it = it->next) {
            pit = (pipe_s *)it->data;
            FD_SET(pit->sock.fd, &fds);
            if (pit->sock.fd > fd_max)
                fd_max = pit->sock.fd;
        }
        if (select(fd_max + 1, &fds, NULL, NULL, NULL) == -1)
            perrend("waitclients() - select()");

        for (it = pipes.head; it; it = it->next) {
            pit = (pipe_s *)it->data;

            if (FD_ISSET(pit->sock.fd, &fds)) {
                newpipe = malloc(sizeof(pipe_s));
                if (!newpipe)
                    perrend("waitclients() - malloc(pipe_s)");

                if (newpipe != NULL) newpipe->sio = pit->sio;
                if (sio_open(&newpipe->sio)) {
                    tcp_refuse(&pit->sock);
                    free(newpipe);
                    continue;
                }
                // accept the connection
                newsock = tcp_accept(&pit->sock);
                // all ok?
                if (newsock) {
                    newpipe->sock = *newsock;
                    free(newsock);
                    newpipe->timeout = pit->timeout;
                    newpipe->net_protocol = pit->net_protocol;
                    newpipe->mutex = pit->mutex;
                    // create the server thread
                    if (thr_create(&thread, 1, serve_pipe, newpipe)) {
                        free(newpipe);
                    }
                }
                else {
                    perror("waitclients() - accept()");
                    free(newpipe);
                }
            }
        }
    }
    return 0;
}

// main routine for the server threads
thr_startfunc_t serve_pipe(void *data)
{
    char sio_buf[BUFSIZ], sock_buf[BUFSIZ];
    char out_buf[BUFSIZ + 2];
    char misc_buf[BUFSIZ];
    int  fd_max, sock_fd;
    int  sio_count, sock_count;
    int  res, port;
    int  websocket_state = 0; // keeps track of websocket state
    int  http_state = 0; // keeps track of http state
    fd_set rfds, wfds;
    pipe_s *pipe = (pipe_s *)data;
    struct timeval tv = { 0, 10000 };
    struct timeval *ptv = &tv;
    DWORD msecs = 0, timeout = pipe->timeout * 1000;
    HANDLE sio_fd;

    if (pipe->net_protocol == 0) { // raw tcp socket, no headers of handshakes
            websocket_state = 10;
            http_state = 10;
    }
    else if (pipe->net_protocol == 1) { // websocket
            websocket_state = 0;
            http_state = 10;
    }
    else { // http
            websocket_state = 10;
            http_state = 0;
    }
    // avoid junk
    memset((void *)sock_buf, 0, sizeof(sock_buf));
    memset((void *)sio_buf,  0, sizeof(sio_buf));
    memset((void *)out_buf,  0, sizeof(out_buf));
    memset((void *)misc_buf, 0, sizeof(misc_buf));

    port = pipe->sio.info.port;
    fprintf(stderr, "server(%d) using protocol %d\n", port, pipe->net_protocol);

    // only proceed if we can lock the mutex
    if (thr_mutex_trylock(pipe->mutex)) {
        error("server(%d) resource is locked", port);
    }
    else {
        sio_count = 0;
        sock_count = 0;
        sio_fd = pipe->sio.fd;
        sock_fd = pipe->sock.fd;
        fd_max = sock_fd;
        msecs = (DWORD)GetTickCount64();
        fprintf(stderr, "server(%d) thread started\n", port);
        
        while (1) {
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_SET(sock_fd, &rfds);
            // only ask for write notification if we have something to write
            if (sio_count > 0) FD_SET(sock_fd, &wfds);
            // wait for read/write events
            res = select(fd_max + 1, &rfds, &wfds, NULL, ptv);
            if (res == -1) {
                perror2("server(%d) select()", port);
                break;
            }
            if (1) {
                // only read input if buffer is empty
                if (sio_count == 0) {

                    //---- kong ----
                    sio_count = sio_read(&pipe->sio, sio_buf, sizeof(sio_buf));
                    if (sio_count < 0) {
                        perror2("server(%d) read(sio) count %d", port, sio_count);
                        break;
                    }
                    //----

                }
            }
            if (websocket_state == 1) {
                // we have header from client, send our key and headers to client.
                sio_count = websocket_generate_headers(sio_buf);
                websocket_state = 2;
            }
            // write to socket possible?
            if (FD_ISSET(sock_fd, &wfds)) {
                if (sio_count > 0) {
                    if (websocket_state == 2) {
                        // websocket handshake done, send data packet to socket with needed extra bytes

                        //---- kong ----
                        memset(out_buf, 0, sizeof(out_buf));
                        memcpy(out_buf + 2, sio_buf, sio_count);
                        out_buf[0] = 0x81;
                        out_buf[1] = sio_count;
                        if ((res = tcp_write(&pipe->sock, out_buf, (size_t)(sio_count + 2))) < 0) {
                            perror2("server(%d) write(sock)", port);
                            break;
                        }
                        sio_count -= res - 2;
                        //----

                    }
                    else if (http_state == 2) {
                        // http connection ready and headers sent, wait for end of line and send HTTP response to client
                        memset(out_buf, 0, sizeof(out_buf));
                        if ((res = tcp_write(&pipe->sock, sio_buf, sio_count)) < 0) {
                            perror2("server(%d) http data sent(sock)", port);
                            break;
                        }
                        if (strchr(sio_buf, '\n')) {
                            sio_count = 0;
                            break;
                        }
                        sio_count -= res;
                    }
                    else {
                        // send raw data
                        if ((res = tcp_write(&pipe->sock, sio_buf, sio_count)) < 0) {
                            perror2("server(%d) write(sock)", port);
                            break;
                        }
                        sio_count -= res;
                    }
                }
            }
            // input from socket?
            if (FD_ISSET(sock_fd, &rfds)) {
                // only read input if buffer is empty
                if (sock_count == 0) {
                    sock_count = tcp_read(&pipe->sock, sock_buf, sizeof(sock_buf));
                    if (sock_count <= 0) {
                        if (sock_count == 0) {
                            fprintf(stderr, "server(%d) EOF from sock\n", port);
                            break;
                        }
                        else
                            break;
                    }
                    if (websocket_state == 0) {
                        websocket_generate_key(sock_buf);
                        // now that we have key, discard header. device in serial port is not interested of it
                        memset(sock_buf, 0, sizeof(sock_buf));
                        sock_count = 0;
                        websocket_state = 1;
                    }
                    else if (http_state == 0) {
                        http_generate_headers( out_buf, 200, "OK", -1 ); 
                        if ((res = tcp_write(&pipe->sock, out_buf, strlen(out_buf))) < 0) {
                            perror2("server(%d) header write(sock)", port);
                            break;
                        }
                        // handle request parameters
                        memset(misc_buf, 0, sizeof(misc_buf));
                        res = http_handle_get( sock_buf, misc_buf );
                        if (res == 0) { // favicon request or similar, we ignore that
                            fprintf(stderr, "server(%d) ignoring get %s\n", port, misc_buf);
                            memset(sock_buf, 0, sizeof(sock_buf));
                            sock_count = 0; sio_count = 0;
                            break;
                        }
                        else {
                            fprintf(stderr, "server(%d) get %s\n", port, misc_buf );
                            memset(sock_buf, 0, sizeof(sock_buf));
                            memcpy(sock_buf, misc_buf, strlen(misc_buf));
                            sock_count = (int)strlen(misc_buf);
                        }
                        http_state = 1;
                    }
                }
                else
                    fprintf(stderr, "socket blocked\n");
            }
            // no socket IO performed?
            if ((!FD_ISSET(sock_fd, &rfds)) && (!FD_ISSET(sock_fd, &wfds))) {
                // break on a time out
                if (GetTickCount64() - msecs > timeout) {
                    fprintf(stderr, "server(%d) timed out\n", port);
                    break;
                }
            }
            else {
                msecs = (DWORD)GetTickCount64();
            }
            if (1) {
                if (sock_count > 0) {
                    if (sock_buf[0] & 0x08 && sock_count > 1 && websocket_state == 2) {
                        fprintf(stderr,"server(%d) close frame from client\n", port);
                        break;
                    }
                    if (sock_buf[0] & 0x81 && sock_count > 6 && websocket_state == 2) {
                        // data from socket to serial, since we are using websocket,
                        // we need to check if data is masked
                        // this is for spec 10
                        websocket_mask_data(sock_buf, out_buf, sock_count);

                        memset(sock_buf, 0, sizeof(sock_buf));
                        memcpy(sock_buf, &out_buf[6], (size_t)(sock_count - 6));
                        sock_count -= 6;
                    }
                    else if (http_state == 1) {
                        // we can ignore this part with HTTP
                        http_state = 2;
                    }

                    //---- kong ----
                    //if ((res = sio_write(&pipe->sio, sock_buf, sock_count)) < 0) {
                    //    perror2("server(%d) write(sio)", port);
                    //    break;
                    //}
                    //sock_count -= res;
                    sock_count = 0;
                    //----
                }
            }
            memset((void *)sock_buf, 0, sizeof(sock_buf) );
        }
        /* unlock our mutex */
        thr_mutex_unlock(pipe->mutex);		
    }
    fprintf(stderr, "server(%d) exiting\n", port);
    fprintf(stderr, "server(%d) releasing serial port\n", port);
    sio_cleanup(&pipe->sio);
    tcp_cleanup(&pipe->sock);
    free(pipe);
    thr_exit((thr_exitcode_t)0);
    return (thr_exitcode_t)0;
}

/**/
