#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum
{
    TOKTYPE_KEYWORD,
    TOKTYPE_IDENTIFIER,
    TOKTYPE_INTLIT,

    TOKTYPE_LPAREN,
    TOKTYPE_RPAREN,
    TOKTYPE_LBRACE,
    TOKTYPE_RBRACE,
    TOKTYPE_LBRACKET,
    TOKTYPE_RBRACKET,

    TOKTYPE_SEMICOLON,
    TOKTYPE_COMMA,

    TOKTYPE_PLUS,
    TOKTYPE_MINUS,
    TOKTYPE_STAR,
    TOKTYPE_SLASH,
    TOKTYPE_PERCENT,

    TOKTYPE_EQUAL,
    TOKTYPE_LESS,
    TOKTYPE_GREATER,

    TOKTYPE_BANG,
    TOKTYPE_AMPERSAND,
    TOKTYPE_PIPE,
    TOKTYPE_CARET,
    TOKTYPE_TILDE,

    TOKTYPE_QUESTION,
    TOKTYPE_COLON,
    TOKTYPE_DOT,
    TOKTYPE_ARROW,

    TOKTYPE_PLUS_PLUS,
    TOKTYPE_MINUS_MINUS,

    TOKTYPE_EQUAL_EQUAL,
    TOKTYPE_NOT_EQUAL,
    TOKTYPE_LESS_EQUAL,
    TOKTYPE_GREATER_EQUAL,

    TOKTYPE_AND_AND,
    TOKTYPE_OR_OR,

    TOKTYPE_SHIFT_LEFT,
    TOKTYPE_SHIFT_RIGHT,

    TOKTYPE_ELLIPSIS
} token_type_t;

extern const char *keywords[];

typedef struct
{
    token_type_t type;
    char *text;
    uint64_t intval;
} token_t;

void init_lexer(void);
int lex(const char *text, token_t *out_tok);
