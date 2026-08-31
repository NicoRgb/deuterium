#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "logger.h"

#define ARRAY_INITIAL_CAPACITY 5

typedef struct
{
    size_t size;
    size_t capacity;
} __array_metadata_t;

#define array_t(type) type *
#define NULL_ARRAY NULL

#define array_create(type)                                                                                    \
    ({                                                                                                        \
        __array_metadata_t *res = malloc(sizeof(type) * ARRAY_INITIAL_CAPACITY + sizeof(__array_metadata_t)); \
        if (!res)                                                                                             \
        {                                                                                                     \
            log_error("failed to allocate memory");                                                           \
            exit(EXIT_FAILURE);                                                                               \
        }                                                                                                     \
                                                                                                              \
        res->size = 0;                                                                                        \
        res->capacity = ARRAY_INITIAL_CAPACITY;                                                               \
                                                                                                              \
        (type *)((uintptr_t)res + sizeof(__array_metadata_t));                                                \
    })

#define array_free(array)                                                                                     \
    do                                                                                                        \
    {                                                                                                         \
        ASSERT(array);                                                                                        \
        __array_metadata_t *metadata = (__array_metadata_t *)((uintptr_t)array - sizeof(__array_metadata_t)); \
        free(metadata);                                                                                       \
        array = NULL_ARRAY;                                                                                   \
    } while (0)

#define array_push(array, element)                                                                                \
    do                                                                                                            \
    {                                                                                                             \
        ASSERT(array);                                                                                            \
        __array_metadata_t *metadata = (__array_metadata_t *)((uintptr_t)array - sizeof(__array_metadata_t));     \
        ASSERT(metadata->capacity != 0);                                                                          \
        ASSERT(metadata->size <= metadata->capacity);                                                             \
        if (metadata->size >= metadata->capacity)                                                                 \
        {                                                                                                         \
            metadata = realloc(metadata, sizeof(array[0]) * metadata->capacity * 2 + sizeof(__array_metadata_t)); \
            if (!metadata)                                                                                        \
            {                                                                                                     \
                log_error("failed to allocate memory");                                                           \
                exit(EXIT_FAILURE);                                                                               \
            }                                                                                                     \
            metadata->capacity *= 2;                                                                              \
            array = (void *)((uintptr_t)metadata + sizeof(__array_metadata_t));                                   \
        }                                                                                                         \
        ASSERT(metadata->size <= metadata->capacity);                                                             \
                                                                                                                  \
        size_t index = metadata->size++;                                                                          \
        array[index] = element;                                                                                   \
    } while (0)

#define array_size(array) ((__array_metadata_t *)((uintptr_t)array - sizeof(__array_metadata_t)))->size
