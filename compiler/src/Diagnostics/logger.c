#include "logger.h"
#include "arguments.h"

#include <stdio.h>
#include <stdarg.h>

typedef enum
{
    log_level_debug = 0,
    log_level_info = 1,
    log_level_warn = 2,
    log_level_error = 3
} log_level_t;

const char *color_table[] =
    {
        "\x1b[0;38;5;8;49m", // grey
        "\x1b[0;96;49m",     // bright cyan
        "\x1b[0;93;49m",     // bright yellow
        "\x1b[1;39;41m",     // bold, red background
        "\033[0m"            // reset
};

const char *log_level_names[] =
    {
        "Debug",
        "Info",
        "Warn",
        "Error"};

static void _log(log_level_t level, const char *fmt, va_list args)
{
    if (level > 3)
        exit(EXIT_FAILURE);

#ifndef ALWAYS_VERBOSE
    compiler_config_t *config = get_global_config();

    if ((!config || config->verbosity != COMPILER_VERBOSITY_ALL) && level < 3)
        return;
#endif

    printf("%s%s: ", color_table[level], log_level_names[level]);
    vprintf(fmt, args);
    printf("%s\n", color_table[4]);
}

void log_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(log_level_debug, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(log_level_info, fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(log_level_warn, fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(log_level_error, fmt, args);
    va_end(args);
}
