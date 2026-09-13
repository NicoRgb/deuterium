#pragma once

#include "lexer.h"

void tokstream_init(const char *text);

size_t tokstream_checkpoint(void);
void tokstream_restore(size_t checkpoint);

token_t *tok_next(void);
token_t *tok_peek(void);
token_t *tok_peek_nth(size_t n);

token_t *tok_expect(token_type_t type);
token_t *tok_expect_n(size_t n, ...);

void tok_free(token_t *tok);

token_t *tok_forge(token_type_t type, char *text);
