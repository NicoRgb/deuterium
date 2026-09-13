#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "logger.h"

#define ARRAY_INITIAL_CAPACITY 20

typedef struct
{
    size_t size;
    size_t capacity;
} __array_metadata_t;

#define array_t(type) type *
#define NULL_ARRAY NULL

#define array_create(type, arr)                                                                               \
    do                                                                                                        \
    {                                                                                                         \
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
        arr = (type *)((uintptr_t)res + sizeof(__array_metadata_t));                                          \
    } while (0)

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

#define array_clear(array)                                                                                    \
    do                                                                                                        \
    {                                                                                                         \
        ASSERT(array);                                                                                        \
        __array_metadata_t *metadata = (__array_metadata_t *)((uintptr_t)array - sizeof(__array_metadata_t)); \
        ASSERT(metadata->capacity != 0);                                                                      \
        metadata->size = 0;                                                                                   \
    } while (0)

#define array_size(array) ((__array_metadata_t *)((uintptr_t)array - sizeof(__array_metadata_t)))->size

#define foreach(array, it) for (size_t _i = 0;                                     \
                                _i < array_size(array) && ((it) = (array)[_i], 1); \
                                ++_i)

#define static_array_t(type) type *
#define NULL_ARRAY NULL

#define static_array_create(type, cap, arr)                                                 \
    do                                                                                      \
    {                                                                                       \
        __array_metadata_t *_res = malloc(sizeof(type) * cap + sizeof(__array_metadata_t)); \
        if (!_res)                                                                          \
        {                                                                                   \
            log_error("failed to allocate memory");                                         \
            exit(EXIT_FAILURE);                                                             \
        }                                                                                   \
                                                                                            \
        _res->size = 0;                                                                     \
        _res->capacity = cap;                                                               \
                                                                                            \
        arr = (type *)((uintptr_t)_res + sizeof(__array_metadata_t));                       \
    } while (0)

#define static_array_free(array) array_free(array)

#define static_array_push(array, element)                                                                     \
    do                                                                                                        \
    {                                                                                                         \
        ASSERT(array);                                                                                        \
        __array_metadata_t *metadata = (__array_metadata_t *)((uintptr_t)array - sizeof(__array_metadata_t)); \
        ASSERT(metadata->capacity != 0);                                                                      \
        ASSERT(metadata->size <= metadata->capacity);                                                         \
        size_t index = metadata->size++;                                                                      \
        array[index] = element;                                                                               \
    } while (0)

#define static_array_clear(array) array_clear(array)
#define static_array_size(array) array_size(array)
