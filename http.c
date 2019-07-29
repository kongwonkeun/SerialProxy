/**/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "error.h"

#define SERVER "serialproxy/1.1"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

int http_generate_headers(char *f, int status, char *title, int length)
{
    time_t now;
    char timebuf[128];

    sprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
    sprintf(f, "%sServer: %s\r\n", f, SERVER);
    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
    sprintf(f, "%sDate: %s\r\n", f, timebuf);
    sprintf(f, "%sContent-Type: text/html\r\n", f);
    if (length >= 0) sprintf(f, "%sContent-Length: %d\r\n", f, length);
    sprintf(f, "%sLast-Modified: %s\r\n", f, timebuf);
    sprintf(f, "%sConnection: close\r\n", f);
    sprintf(f, "%s\r\n", f);
    return length;
}

int http_handle_get(char *f, char *out_buf)
{
    char *token[80];
    char *req_token[80];
    char i = 0;

    token[0] = strtok(f, "\n");
    while (token[i] != NULL) {
        i++;
        token[i] = strtok(NULL, "\n");
    }
    i = 0;
    req_token[0] = strtok(token[0], " ");
    while (req_token[i] != NULL) {
        i++;
        req_token[i] = strtok(NULL, " ");
    }
    sprintf(out_buf, "%s", strtok(req_token[1], "/"));
    if (strstr(out_buf, "favicon.ico")) {
        return 0;
    }
    return 1;
}

/**/
