#include <strdup.h>
#include <stdlib.h>
#include <string.h>

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
