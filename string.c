/**/

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>

#include "string.h"

int str_assign(char **pstr, char *text)
{
    char *p;
    
    if (!text) {
        free(*pstr);
        *pstr = NULL;
        return 0;
    }
    p = realloc(*pstr, strlen(text) + 1);
    if (!p)
        return -1;
    *pstr = p;
    strcpy(p, text);
    return 0;
}

int str_cat(char **pstr, char *text)
{
    char *p = realloc(*pstr, strlen(*pstr) + strlen(text) + 1);
    if (!p)
        return -1;
    *pstr = p;
    strcat(p, text);
    return 0;
}

int str_striptrail(char *str)
{
    char *p;
    
    for (p = str + strlen(str) - 1; p >= str; p--) {
        if (*p != ' ') {
            *(p + 1) = '\0';
            return 0;
        }
    }
    return 0;
}

void str_cleanup(char **pstr)
{
    if (*pstr) {
        free(*pstr);
        *pstr = NULL;
    }
}

/**/
