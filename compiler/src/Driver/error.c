#include "error.h"
#include "array.h"
#include "ansi.h"

#include <string.h>

const char *g_filename;
const char *g_error_text;

typedef struct
{
    position_t start;
    position_t end;

    char message[MAX_ERROR_MSG];
} error_t;

array_t(error_t) g_errors = NULL_ARRAY;

void init_errors(const char *filename, const char *text)
{
    if (g_errors == NULL_ARRAY)
        array_create(error_t, g_errors);
    else
        array_clear(g_errors);

    g_filename = filename;
    g_error_text = text;
}

int has_errors(void)
{
    return array_size(g_errors) > 0;
}

static const char *get_line(size_t line, size_t *length)
{
    const char *p = g_error_text;

    for (size_t i = 0; i < line; i++)
    {
        p = strchr(p, '\n');

        if (!p)
            return NULL;

        p++;
    }

    const char *end = strchr(p, '\n');

    if (!end)
        end = p + strlen(p);

    *length = (size_t)(end - p);
    return p;
}

static size_t digits(size_t n)
{
    size_t count = 1;

    while (n >= 10)
    {
        n /= 10;
        count++;
    }

    return count;
}

void emit_errors(void)
{
    for (size_t i = 0; i < array_size(g_errors); i++)
    {
        error_t *err = &g_errors[i];

        size_t line_length = 0;
        const char *line = get_line(err->start.row, &line_length);

        if (!line)
            continue;

        size_t start_column = err->start.col;

        if (start_column > line_length)
            start_column = line_length;

        size_t end_column;

        if (err->end.row == err->start.row)
            end_column = err->end.col;
        else
            end_column = line_length;

        if (end_column <= start_column)
            end_column = start_column + 1;

        if (end_column > line_length)
            end_column = line_length;

        size_t marker_length = end_column - start_column;

        if (marker_length == 0)
        {
            if (start_column < line_length)
                marker_length = 1;
            else if (line_length > 0)
            {
                start_column = line_length - 1;
                marker_length = 1;
            }
            else
            {
                marker_length = 1;
            }
        }

        size_t line_number_width = digits(err->start.row + 1);

        printf(ANSI_BOLD ANSI_RED "error:" ANSI_RESET " %s\n", err->message);
        printf(ANSI_GRAY "  --> " ANSI_RESET "%s:%zu:%zu\n", g_filename, err->start.row + 1, err->start.col + 1);
        printf(ANSI_GRAY "%*s |\n" ANSI_RESET, (int)(line_number_width + 2), "");
        printf(ANSI_GRAY "%*zu | " ANSI_RESET, (int)line_number_width, err->start.row + 1);

        fwrite(line, 1, line_length, stdout);
        putchar('\n');

        printf(ANSI_GRAY "%*s | " ANSI_RESET, (int)line_number_width, "");

        for (size_t j = 0; j < start_column && j < line_length; j++)
        {
            if (line[j] == '\t')
                putchar('\t');
            else
                putchar(' ');
        }

        printf(ANSI_BOLD ANSI_RED "^");

        for (size_t j = 1; j < marker_length; j++)
            putchar('~');

        printf(ANSI_RESET "\n");
        printf(ANSI_GRAY "%*s |\n" ANSI_RESET, (int)(line_number_width + 2), "");
    }
}

void push_error(position_t *start, position_t *end, const char *msg)
{
    error_t err;
    err.start = *start;
    err.end = *end;
    strncpy(err.message, msg, MAX_ERROR_MSG);

    array_push(g_errors, err);
}
