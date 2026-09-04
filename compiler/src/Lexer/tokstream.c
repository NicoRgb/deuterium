#include "tokstream.h"
#include "array.h"

#include <string.h>

#define TOKEN_QUEUE_INITIAL_CAPACITY 8

token_t **token_queue = NULL;
size_t queue_size = 0;
size_t queue_capacity = 0;

static void queue_init(void)
{
    queue_capacity = TOKEN_QUEUE_INITIAL_CAPACITY;
    queue_size = 0;
    token_queue = malloc(queue_capacity * sizeof(token_t *));
    if (!token_queue)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }
}

static void queue_push(token_t *tok)
{
    ASSERT(tok);
    ASSERT(token_queue);

    if (queue_size >= queue_capacity)
    {
        queue_capacity *= 2;

        token_queue = realloc(token_queue, queue_capacity * sizeof(token_t *));
        if (!token_queue)
        {
            log_error("failed to allocate memory");
            exit(EXIT_FAILURE);
        }
    }

    token_queue[queue_size++] = tok;
}

static token_t *queue_pop(void)
{
    ASSERT(token_queue);
    ASSERT(queue_size > 0);

    token_t *res = token_queue[0];

    size_t i = --queue_size;
    while (i--)
    {
        res[i] = res[i + 1];
    }

    return res;
}

const char *g_text = NULL;

void tokstream_init(const char *text)
{
    g_text = text;
}

token_t *tok_next(void)
{
    ASSERT(g_text);
    if (!token_queue)
        queue_init();

    if (queue_size > 0)
    {
        return queue_pop();
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

    return tok;
}

token_t *tok_peek(void)
{
    ASSERT(g_text);
    if (!token_queue)
        queue_init();

    if (queue_size > 0)
    {
        return token_queue[0];
    }

    token_t *tok = malloc(sizeof(token_t));
    if (!tok)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    lex(g_text, tok);
    queue_push(tok);

    return tok;
}

token_t *tok_peek_nth(size_t n)
{
    ASSERT(g_text);
    if (!token_queue)
        queue_init();

    if (queue_size >= n)
    {
        return token_queue[n];
    }

    token_t *res = NULL;

    for (size_t i = queue_size; i < n; i++)
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

        queue_push(tok);
        res = tok;
    }

    return res;
}

int tok_expect(token_type_t type)
{
    token_t *tok = tok_peek();
    if (!tok)
        return 0;

    if (tok->type != type)
    {
        char msg[MAX_ERROR_MSG];
        snprintf(msg, MAX_ERROR_MSG, "expected %s", token_type_error_names[type]);

        push_error(&tok->start, &tok->end, msg);
        return 0;
    }

    tok_next();
    tok_free(tok);

    return 1;
}

void tok_free(token_t *tok)
{
    ASSERT(tok);

#ifndef NDEBUG
    for (size_t i = 0; i < queue_size; i++)
    {
        if (token_queue[i] == tok)
        {
            log_error("tok_free called on token still in queue");
            exit(EXIT_FAILURE);
        }
    }
#endif

    if (tok->type == TOKTYPE_IDENTIFIER || tok->type == TOKTYPE_INTLIT)
        free(tok->text);

    free(tok);
}
