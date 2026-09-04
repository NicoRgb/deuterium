#pragma once

#include <stdint.h>
#include <stddef.h>

#include "error.h"

typedef enum
{
    TOKTYPE_EOF,
    TOKTYPE_INVALID,

    TOKTYPE_IDENTIFIER,
    TOKTYPE_INTLIT,

    TOKTYPE_KW_INT,
    TOKTYPE_KW_VOID,
    TOKTYPE_KW_RETURN,
    TOKTYPE_KW_IF,
    TOKTYPE_KW_ELSE,
    TOKTYPE_KW_WHILE,
    TOKTYPE_KW_FOR,

    TOKTYPE_LPAREN,
    TOKTYPE_RPAREN,

    TOKTYPE_LBRACE,
    TOKTYPE_RBRACE,

    TOKTYPE_LBRACKET,
    TOKTYPE_RBRACKET,

    TOKTYPE_SEMICOLON,
    TOKTYPE_COMMA,

    TOKTYPE_DOT,
    TOKTYPE_ARROW,

    TOKTYPE_QUESTION,
    TOKTYPE_COLON,

    TOKTYPE_PLUS,
    TOKTYPE_MINUS,
    TOKTYPE_STAR,
    TOKTYPE_SLASH,
    TOKTYPE_PERCENT,

    TOKTYPE_PLUS_PLUS,
    TOKTYPE_MINUS_MINUS,

    TOKTYPE_EQUAL,

    TOKTYPE_MUL_ASSIGN,
    TOKTYPE_DIV_ASSIGN,
    TOKTYPE_MOD_ASSIGN,

    TOKTYPE_ADD_ASSIGN,
    TOKTYPE_SUB_ASSIGN,

    TOKTYPE_LEFT_ASSIGN,
    TOKTYPE_RIGHT_ASSIGN,

    TOKTYPE_AND_ASSIGN,
    TOKTYPE_XOR_ASSIGN,
    TOKTYPE_OR_ASSIGN,

    TOKTYPE_EQUAL_EQUAL,
    TOKTYPE_NOT_EQUAL,

    TOKTYPE_LESS,
    TOKTYPE_GREATER,

    TOKTYPE_LESS_EQUAL,
    TOKTYPE_GREATER_EQUAL,

    TOKTYPE_AND_AND,
    TOKTYPE_OR_OR,

    TOKTYPE_BANG,

    TOKTYPE_AMPERSAND,
    TOKTYPE_PIPE,
    TOKTYPE_CARET,
    TOKTYPE_TILDE,

    TOKTYPE_SHIFT_LEFT,
    TOKTYPE_SHIFT_RIGHT,

    TOKTYPE_ELLIPSIS
} token_type_t;

typedef struct
{
    token_type_t type;
    char *text;

    position_t start;
    position_t end;
} token_t;

extern const char *token_type_names[];
extern const char *token_type_error_names[];

void init_lexer(void);
int lex(const char *text, token_t *out_tok);
