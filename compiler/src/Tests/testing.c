#include <testing.h>
#include <array.h>
#include <strdup.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "logger.h"
#include "ansi.h"

static test_t *create_test(test_func_t func, const char *name)
{
    ASSERT(func);
    ASSERT(name);

    test_t *res = malloc(sizeof(test_t));
    if (!res)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    memset(res, 0, sizeof(test_t));

    res->func = func;
    res->name = strdup(name);

    res->status = TEST_STATUS_IDLE;

    pthread_mutex_init(&res->mutex_lock, NULL);

    return res;
}

static void free_test(test_t *test)
{
    ASSERT(test);

    if (test->name)
        free(test->name);

    free(test);
}

array_t(test_t *) registered_tests = NULL_ARRAY;

void register_test(test_func_t func, const char *name)
{
    ASSERT(func);
    ASSERT(name);

    if (!registered_tests)
        array_create(test_t *, registered_tests);

    test_t *test = create_test(func, name);
    array_push(registered_tests, test);
}

void free_tests(void)
{
    if (!registered_tests)
        return;

    for (size_t i = 0; i < array_size(registered_tests); i++)
    {
        if (registered_tests[i])
            free_test(registered_tests[i]);
    }

    array_free(registered_tests);
}

static void run_test(test_t *test)
{
    ASSERT(test);

    pthread_mutex_lock(&test->mutex_lock);
    test->status = TEST_STATUS_RUNNING;
    test->start_time = clock();
    pthread_mutex_unlock(&test->mutex_lock);

    test_result_t res = test->func();
    test->status = res.status;
    test->error_message = res.error_str;

    pthread_mutex_lock(&test->mutex_lock);
    if (test->status == TEST_STATUS_RUNNING)
    {
        // TODO: warning
        test->status = TEST_STATUS_FAILED;
    }

    test->end_time = clock();
    test->time_taken = test->end_time - test->start_time;
    pthread_mutex_unlock(&test->mutex_lock);
}

static atomic_size_t current_test_index = 0;
static atomic_int num_threads_working = 0;
static void setup_threading(void)
{
    atomic_init(&current_test_index, 0);
    atomic_init(&num_threads_working, 0);
}

static void *worker_thread_func(void *arg)
{
    atomic_fetch_add(&num_threads_working, 1);

    (void)arg;
    while (1)
    {
        size_t index = atomic_fetch_add(&current_test_index, 1);
        if (index >= array_size(registered_tests))
        {
            break;
        }

        test_t *test = registered_tests[index];
        if (test)
        {
            run_test(test);
        }
    }

    atomic_fetch_sub(&num_threads_working, 1);
    return NULL;
}

static void dashboard_begin(void)
{
    printf("\033[?1049h");
    printf("\033[?25l");
    printf("\033[2J");
    printf("\033[H");

    fflush(stdout);
}

static void dashboard_end(void)
{
    printf("\033[?25h");
    printf("\033[?1049l");

    fflush(stdout);
}

static int terminal_height(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        return 24;

    return ws.ws_row;
}

static int terminal_width(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        return 80;

    return ws.ws_col;
}

static const char *status_string(test_status_t status)
{
    switch (status)
    {
    case TEST_STATUS_IDLE:
        return "WAIT";

    case TEST_STATUS_RUNNING:
        return "RUN";

    case TEST_STATUS_PASSED:
        return "PASS";

    case TEST_STATUS_FAILED:
        return "FAIL";

    default:
        return "????";
    }
}

static const char *status_color(test_status_t status)
{
    switch (status)
    {
    case TEST_STATUS_IDLE:
        return ANSI_GRAY;

    case TEST_STATUS_RUNNING:
        return ANSI_YELLOW;

    case TEST_STATUS_PASSED:
        return ANSI_GREEN;

    case TEST_STATUS_FAILED:
        return ANSI_RED;

    default:
        return ANSI_RESET;
    }
}

static void format_duration(clock_t ticks, char *buf, size_t size)
{
    double ms = ((double)ticks / CLOCKS_PER_SEC) * 1000.0;

    if (ms < 1.0)
    {
        snprintf(buf, size, "%.2f ms", ms);
    }
    else if (ms < 1000.0)
    {
        snprintf(buf, size, "%.2f ms", ms);
    }
    else if (ms < 60000.0)
    {
        snprintf(buf, size, "%.2f s", ms / 1000.0);
    }
    else
    {
        uint64_t seconds = (uint64_t)(ms / 1000.0);
        uint64_t minutes = seconds / 60;
        seconds %= 60;

        snprintf(buf, size, "%llum %02llus",
                 (unsigned long long)minutes,
                 (unsigned long long)seconds);
    }
}

typedef struct
{
    size_t passed;
    size_t failed;
    size_t running;
    size_t waiting;
    size_t completed;
} test_summary_t;

static void render_dashboard(clock_t dashboard_start_time)
{
    int height = terminal_height();
    int width = terminal_width();

    (void)width;

    size_t num_tests = array_size(registered_tests);

    test_summary_t summary = {0};

    for (size_t i = 0; i < num_tests; i++)
    {
        test_t *test = registered_tests[i];

        pthread_mutex_lock(&test->mutex_lock);

        switch (test->status)
        {
        case TEST_STATUS_PASSED:
            summary.passed++;
            summary.completed++;
            break;

        case TEST_STATUS_FAILED:
            summary.failed++;
            summary.completed++;
            break;

        case TEST_STATUS_RUNNING:
            summary.running++;
            break;

        case TEST_STATUS_IDLE:
            summary.waiting++;
            break;
        }

        pthread_mutex_unlock(&test->mutex_lock);
    }

    /*
     * Clear the alternate screen and return to its top.
     */
    printf("\033[H");
    printf("\033[2J");

    printf(ANSI_BOLD
           "+ Test Results "
           "---------------------------------------------------------------+" ANSI_RESET "\n\n");

    /*
     * Progress.
     */
    printf("  Progress: [");

    int progress_width = 40;
    int progress = num_tests
                       ? (int)((summary.completed * progress_width) / num_tests)
                       : 0;

    for (int i = 0; i < progress_width; i++)
        putchar(i < progress ? '=' : '.');

    printf("] %zu/%zu\n\n",
           summary.completed,
           num_tests);

    /*
     * Test rows.
     */
    int available_rows = height - 10;

    if (available_rows < 1)
        available_rows = 1;

    size_t shown = num_tests;

    if ((size_t)available_rows < shown)
        shown = (size_t)available_rows;

    for (size_t i = 0; i < shown; i++)
    {
        test_t *test = registered_tests[i];

        pthread_mutex_lock(&test->mutex_lock);

        test_status_t status = test->status;

        char duration[32];

        if (status == TEST_STATUS_RUNNING)
        {
            format_duration(
                clock() - test->start_time,
                duration,
                sizeof(duration));
        }
        else if (status == TEST_STATUS_PASSED ||
                 status == TEST_STATUS_FAILED)
        {
            format_duration(
                test->time_taken,
                duration,
                sizeof(duration));
        }
        else
        {
            snprintf(duration, sizeof(duration), "--");
        }

        printf("  %s%-4s%s  %-10s " ANSI_GRAY "%-20s" ANSI_RESET " %10s\n",
               status_color(status),
               status_string(status),
               ANSI_RESET,
               test->name ? test->name : "unnamed",
               test->error_message ? test->error_message : "",
               duration);

        pthread_mutex_unlock(&test->mutex_lock);
    }

    /*
     * Summary.
     */
    printf("\n");

    printf("  " ANSI_GREEN "Passed: %-4zu" ANSI_RESET
           "  " ANSI_RED "Failed: %-4zu" ANSI_RESET
           "  " ANSI_YELLOW "Running: %-4zu" ANSI_RESET
           "  " ANSI_GRAY "Waiting: %-4zu" ANSI_RESET "\n",
           summary.passed,
           summary.failed,
           summary.running,
           summary.waiting);

    char total_time[32];

    format_duration(
        clock() - dashboard_start_time,
        total_time,
        sizeof(total_time));

    printf("\n");
    printf("  Total: %-4zu                              Elapsed: %s\n",
           num_tests,
           total_time);

    printf("\n");
    printf(ANSI_BOLD
           "+-----------------------------------------------------------------------------+" ANSI_RESET "\n");

    fflush(stdout);
}

void run_tests(void)
{
    if (!registered_tests)
        return;

    setup_threading();

    size_t num_tests = array_size(registered_tests);

    pthread_t threads[MAX_TEST_THREADS];
    size_t active_threads = num_tests < MAX_TEST_THREADS ? num_tests : MAX_TEST_THREADS;

    for (size_t i = 0; i < active_threads; i++)
    {
        if (pthread_create(&threads[i], NULL, worker_thread_func, NULL) != 0)
        {
            log_error("failed to create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    clock_t dashboard_start_time = clock();
    dashboard_begin();
    int threads_running = 1;
    while (threads_running)
    {
        threads_running = 0;

        for (size_t i = 0; i < num_tests; i++)
        {
            test_t *test = registered_tests[i];
            pthread_mutex_lock(&test->mutex_lock);

            if (test->status == TEST_STATUS_IDLE || test->status == TEST_STATUS_RUNNING)
                threads_running = 1;

            pthread_mutex_unlock(&test->mutex_lock);
        }

        render_dashboard(dashboard_start_time);

        if (threads_running)
        {
            usleep(50000);
        }
    }
    dashboard_end();

    for (size_t i = 0; i < active_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    render_dashboard(dashboard_start_time);
}
