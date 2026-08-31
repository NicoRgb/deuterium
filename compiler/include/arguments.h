#pragma once

#include <array.h>

typedef enum
{
    COMPILER_MODE_VERSION,
    COMPILER_MODE_COMPILE
} compiler_mode_t;

typedef enum
{
    COMPILER_VERBOSITY_ALL, // print internal debug logs
    COMPILER_VERBOSITY_NORMAL,
} compiler_verbosity_t;

typedef struct
{
    compiler_mode_t mode;
    compiler_verbosity_t verbosity;

    char *outfile;
    array_t(char *) infiles;
} compiler_config_t;

compiler_config_t *get_global_config(void); // in main.c

void parse_arguments(int argc, char *argv[], compiler_config_t *out_config);
