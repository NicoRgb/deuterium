#pragma once

#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define MAX_TEST_THREADS 3

typedef enum
{
    TEST_STATUS_IDLE,
    TEST_STATUS_RUNNING,
    TEST_STATUS_PASSED,
    TEST_STATUS_FAILED
} test_status_t;

typedef test_status_t (*test_func_t)(void);

typedef struct
{
    pthread_mutex_t mutex_lock;

    test_status_t status;
    test_func_t func;

    clock_t start_time;
    clock_t end_time;
    uint64_t time_taken;

    char *name;
    char *description;
    char *error_message;
} test_t;

void register_test(test_func_t func, const char *name, const char *desc);
void free_tests(void);
void run_tests(void);

#define DECLARE_TEST(name, desc)                                        \
    static test_status_t __test_##name(void);                           \
    __attribute__((constructor)) static void register_test_##name(void) \
    {                                                                   \
        register_test(__test_##name, #name, desc);                      \
    }                                                                   \
    static test_status_t __test_##name(void)
