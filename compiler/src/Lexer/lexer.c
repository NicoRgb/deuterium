#include "lexer.h"
#include "logger.h"
#include "error.h"

#include <ctype.h>
#include <string.h>

const char *keywords[] = {"auto", "break", "case", "char", "const", "continue",
                          "default", "do", "double", "else", "enum", "extern",
                          "float", "for", "goto", "if", "inline", "int", "long",
                          "register", "restrict", "return", "short", "signed",
                          "sizeof", "static", "struct", "switch", "typedef",
                          "union", "unsigned", "void", "volatile", "while",
                          "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex",
                          "_Generic", "_Imaginary", "_Noreturn", "_Static_assert",
                          "_Thread_local"};

const char *token_type_names[] = {
    "KEYWORD", "IDENTIFIER", "INTLIT", "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACKET", "RBRACKET", "SEMICOLON", "COMMA", "PLUS",
    "MINUS", "STAR", "SLASH", "PERCENT", "EQUAL", "LESS", "GREATER", "BANG", "AMPERSAND", "PIPE", "CARET", "TILDE", "QUESTION", "COLON",
    "DOT", "ARROW", "PLUS_PLUS", "MINUS_MINUS", "EQUAL_EQUAL", "NOT_EQUAL", "LESS_EQUAL", "GREATER_EQUAL", "AND_AND", "OR_OR", "SHIFT_LEFT",
    "SHIFT_RIGHT", "ELLIPSIS"};

const char *token_type_error_names[] = {
    "keyword", "identifier", "integer literal", "(", ")", "{", "}", "[", "]", ";", ",", "+",
    "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "?", ":",
    ".", "->", "++", "--", "==", "!=", "<=", ">=", "&&", "||", "<<",
    ">>", "..."};

static void populate_token(token_type_t type, char *text, token_t *out_token)
{
    ASSERT(out_token);
    out_token->type = type;
    out_token->text = text;
    out_token->intval = 0;
}

#define INITIAL_IDENTIFIER_SIZE 16

size_t g_lex_index, g_lex_col, g_lex_row;

static char *lex_identifier(const char *text)
{
    ASSERT(isalpha(text[g_lex_index]));

    char *identifier = malloc(INITIAL_IDENTIFIER_SIZE);
    if (!identifier)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    size_t identifier_capacity = INITIAL_IDENTIFIER_SIZE;
    size_t identifier_size = 0;

    while (isalpha(text[g_lex_index]) || isdigit(text[g_lex_index]) || text[g_lex_index] == '_')
    {
        if (identifier_size >= (identifier_capacity - 1))
        {
            identifier_capacity *= 2;
            identifier = realloc(identifier, identifier_capacity);
            if (!identifier)
            {
                log_error("failed to allocate memory");
                exit(EXIT_FAILURE);
            }
        }

        identifier[identifier_size++] = text[g_lex_index++];
        g_lex_col++;
    }

    identifier[identifier_size] = 0;
    return identifier;
}

void init_lexer(void)
{
    g_lex_index = 0;
    g_lex_col = 0;
    g_lex_row = 0;
}

int lex(const char *text, token_t *out_tok)
{
    ASSERT(text);

    while (text[g_lex_index])
    {
        if (text[g_lex_index] == '\n')
        {
            g_lex_index++;
            g_lex_col = 0;
            g_lex_row++;
        }
        else if (isspace(text[g_lex_index]))
        {
            g_lex_index++;
            g_lex_col++;
            continue;
        }
        else if (isalpha(text[g_lex_index]))
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};
            char *identifier = lex_identifier(text);

            token_type_t type = TOKTYPE_IDENTIFIER;
            for (size_t i = 0; i < (sizeof(keywords) / sizeof(keywords[0])); i++)
            {
                if (strcmp(identifier, keywords[i]) == 0)
                {
                    type = TOKTYPE_KEYWORD;
                    break;
                }
            }

            populate_token(type, identifier, out_tok);

            out_tok->start.col = start.col;
            out_tok->start.row = start.row;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;

            return 1;
        }
        else if (text[g_lex_index] == '(')
        {
            populate_token(TOKTYPE_LPAREN, "(", out_tok);
            out_tok->start.col = g_lex_col;
            out_tok->start.row = g_lex_row;

            g_lex_index++;
            g_lex_col++;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;
            return 1;
        }
        else if (text[g_lex_index] == ')')
        {
            populate_token(TOKTYPE_RPAREN, ")", out_tok);
            out_tok->start.col = g_lex_col;
            out_tok->start.row = g_lex_row;

            g_lex_index++;
            g_lex_col++;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;
            return 1;
        }
        else if (text[g_lex_index] == '{')
        {
            populate_token(TOKTYPE_LBRACE, "{", out_tok);
            out_tok->start.col = g_lex_col;
            out_tok->start.row = g_lex_row;

            g_lex_index++;
            g_lex_col++;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;
            return 1;
        }
        else if (text[g_lex_index] == '}')
        {
            populate_token(TOKTYPE_RBRACE, "}", out_tok);
            out_tok->start.col = g_lex_col;
            out_tok->start.row = g_lex_row;

            g_lex_index++;
            g_lex_col++;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;
            return 1;
        }
        else if (text[g_lex_index] == ';')
        {
            populate_token(TOKTYPE_SEMICOLON, ";", out_tok);
            out_tok->start.col = g_lex_col;
            out_tok->start.row = g_lex_row;

            g_lex_index++;
            g_lex_col++;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;
            return 1;
        }
        else
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};
            g_lex_index++;
            g_lex_col++;
            position_t end = {.col = g_lex_col, .row = g_lex_row};

            push_error(&start, &end, "unrecognized character");
            continue;
        }
    }

    return 0;
}
