#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "strutils.h"
#include "logger.h"

char *strdup(const char *s)
{
    size_t len = strlen(s);

    char *res = malloc(len + 1);
    if (!res)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    strcpy(res, s);
    return res;
}

char *strcat_alloc(const char *a, const char *b)
{
    size_t len = strlen(a) + strlen(b) + 1;
    char *res = malloc(len);
    strcpy(res, a);
    strcpy((char *)((uintptr_t)res + strlen(a)), b);
    return res;
}
