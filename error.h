#ifndef MYERROR_H
#define MYERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

void error(const char *fmt, ...);
void errend(const char *fmt, ...);
void perror2(const char *fmt, ...);
void perrend(const char *fmt, ...);

#endif /* MYERROR_H */
