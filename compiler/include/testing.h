#pragma once

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define MAX_TEST_THREADS 3

typedef enum
{
    TEST_STATUS_IDLE,
    TEST_STATUS_RUNNING,
    TEST_STATUS_PASSED,
    TEST_STATUS_FAILED
} test_status_t;

typedef struct
{
    const char *error_str;
    test_status_t status;
} test_result_t;

#define TEST_ASSERT_A(a)                                        \
    do                                                          \
    {                                                           \
        if (!(a))                                               \
        {                                                       \
            test_result_t res = {.error_str = #a,               \
                                 .status = TEST_STATUS_FAILED}; \
            return res;                                         \
        }                                                       \
    } while (0)

#define TEST_ASSERT_AB(a, b)                                    \
    do                                                          \
    {                                                           \
        if (a != b)                                             \
        {                                                       \
            test_result_t res = {.error_str = #a " != " #b,     \
                                 .status = TEST_STATUS_FAILED}; \
            return res;                                         \
        }                                                       \
    } while (0)

#define TEST_ASSERT_STR(a, b)                                                \
    do                                                                       \
    {                                                                        \
        if (strcmp(a, b) != 0)                                               \
        {                                                                    \
            test_result_t res = {.error_str = "strcmp(" #a ", " #b ") != 0", \
                                 .status = TEST_STATUS_FAILED};              \
            return res;                                                      \
        }                                                                    \
    } while (0)

#define TEST_FAIL(msg)                                      \
    do                                                      \
    {                                                       \
        test_result_t res = {.error_str = msg,              \
                             .status = TEST_STATUS_FAILED}; \
        return res;                                         \
    } while (0)

#define TEST_SUCCESS()                                      \
    do                                                      \
    {                                                       \
        test_result_t res = {.error_str = "",               \
                             .status = TEST_STATUS_PASSED}; \
        return res;                                         \
    } while (0)

typedef test_result_t (*test_func_t)(void);

typedef struct
{
    pthread_mutex_t mutex_lock;

    test_status_t status;
    test_func_t func;

    clock_t start_time;
    clock_t end_time;
    uint64_t time_taken;

    char *name;
    const char *error_message;
} test_t;

void register_test(test_func_t func, const char *name);
void free_tests(void);
void run_tests(void);

#define DECLARE_TEST(name)                                              \
    static test_result_t __test_##name(void);                           \
    __attribute__((constructor)) static void register_test_##name(void) \
    {                                                                   \
        register_test(__test_##name, #name);                            \
    }                                                                   \
    static test_result_t __test_##name(void)
