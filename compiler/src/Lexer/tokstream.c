#include "tokstream.h"
#include "array.h"

#include <string.h>
#include <stdarg.h>

array_t(token_t *) token_buffer = NULL_ARRAY;
size_t token_head_index = 0;

const char *g_text = NULL;

void tokstream_init(const char *text)
{
    g_text = text;
    token_head_index = 0;
    array_create(token_t *, token_buffer);
}

size_t tokstream_checkpoint(void)
{
    return token_head_index;
}

void tokstream_restore(size_t checkpoint)
{
    token_head_index = checkpoint;
}

token_t *tok_next(void)
{
    ASSERT(g_text);
    ASSERT(token_buffer);

    if (array_size(token_buffer) > token_head_index)
    {
        return token_buffer[token_head_index++];
    }

    token_t *tok = malloc(sizeof(token_t));
    if (!tok)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    if (!lex(g_text, tok))
    {
        return NULL;
    }

    array_push(token_buffer, tok);
    token_head_index++;

    return tok;
}

token_t *tok_peek(void)
{
    ASSERT(g_text);
    ASSERT(token_buffer);

    if (array_size(token_buffer) > token_head_index)
    {
        return token_buffer[token_head_index];
    }

    token_t *tok = malloc(sizeof(token_t));
    if (!tok)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    lex(g_text, tok);
    array_push(token_buffer, tok);

    return tok;
}

token_t *tok_peek_nth(size_t n)
{
    ASSERT(g_text);
    ASSERT(token_buffer);

    if (array_size(token_buffer) > token_head_index + n - 1)
    {
        return token_buffer[token_head_index + n - 1];
    }

    token_t *res = NULL;

    for (size_t i = 0; i < n; i++)
    {
        token_t *tok = malloc(sizeof(token_t));
        if (!tok)
        {
            log_error("failed to allocate memory");
            exit(EXIT_FAILURE);
        }

        if (!lex(g_text, tok))
        {
            return NULL;
        }

        array_push(token_buffer, tok);
        res = tok;
    }

    return res;
}

token_t *tok_expect(token_type_t type)
{
    token_t *tok = tok_peek();
    if (!tok)
        return 0;

    if (tok->type != type)
    {
        char msg[MAX_ERROR_MSG];
        snprintf(msg, MAX_ERROR_MSG, "expected %s", token_type_error_names[type]);

        push_error(&tok->start, &tok->end, msg);
        return NULL;
    }

    return tok_next();
}

token_t *tok_expect_n(size_t n, ...)
{
    token_t *tok = tok_peek();
    if (!tok)
        return 0;

    char msg[MAX_ERROR_MSG];
    strcpy(msg, "expected ");

    va_list args;
    va_start(args, n);
    for (size_t i = 0; i < n; i++)
    {
        token_type_t type = va_arg(args, token_type_t);
        if (tok->type == type)
        {
            va_end(args);
            return tok_next();
        }

        size_t _n = MAX_ERROR_MSG - strlen(msg) - 1;
        strncat(msg, token_type_error_names[type], _n);
        if (i < n - 1)
            strncat(msg, ", ", _n);
    }
    va_end(args);

    push_error(&tok->start, &tok->end, msg);
    return NULL;
}

void tok_free(token_t *tok)
{
    ASSERT(tok);

#ifndef NDEBUG
    for (size_t i = 0; i < array_size(token_buffer); i++)
    {
        if (token_buffer[i] == tok)
        {
            log_error("tok_free called on token still in buffer");
            exit(EXIT_FAILURE);
        }
    }
#endif

    if (tok->type == TOKTYPE_IDENTIFIER || tok->type == TOKTYPE_INTLIT)
        free(tok->text);

    free(tok);
}
