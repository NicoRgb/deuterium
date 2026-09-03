#pragma once

#include <stddef.h>

typedef struct
{
    size_t col;
    size_t row;
} position_t;

#define MAX_ERROR_MSG 32

void init_errors(const char *filename, const char *text);

int has_errors(void);
void emit_errors(void);
void push_error(position_t *start, position_t *end, const char *msg);
