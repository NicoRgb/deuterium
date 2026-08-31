#pragma once

#include <stdlib.h>

void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

#define LOG_DEBUG(fmt, ...) log_debug(fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) log_info(fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) log_warn(fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_error(fmt, __VA_ARGS__)

#define ASSERT(condition)                                                                   \
    do                                                                                      \
    {                                                                                       \
        if (!(condition))                                                                   \
        {                                                                                   \
            log_error("assertion failure at %s:%lld (%s)", __FILE__, __LINE__, #condition); \
            abort();                                                                        \
        }                                                                                   \
    } while (0)

#define ASSERT_MSG(condition, msg)                                                   \
    do                                                                               \
    {                                                                                \
        if (!(condition))                                                            \
        {                                                                            \
            log_error("assertion failure at %s:%lld (%s)", __FILE__, __LINE__, msg); \
            abort();                                                                 \
        }                                                                            \
    } while (0)
