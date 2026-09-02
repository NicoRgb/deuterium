#pragma once

#include "lexer.h"

void tokstream_init(const char *text);

token_t *tok_next(void);
token_t *tok_peek(void);
token_t *tok_peek_nth(size_t n);

int tok_expect(token_type_t type);
int tok_expect_kw(const char *kw);

void tok_free(token_t *tok);
