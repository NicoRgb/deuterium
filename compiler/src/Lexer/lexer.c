#include "lexer.h"
#include "logger.h"
#include "error.h"

#include <ctype.h>
#include <string.h>

const char *token_type_names[] = {
    "TOKTYPE_EOF", "TOKTYPE_INVALID", "TOKTYPE_IDENTIFIER", "TOKTYPE_INTLIT",
    "TOKTYPE_KW_INT", "TOKTYPE_KW_VOID", "TOKTYPE_KW_RETURN", "TOKTYPE_KW_IF",
    "TOKTYPE_KW_ELSE", "TOKTYPE_KW_WHILE", "TOKTYPE_KW_FOR", "TOKTYPE_LPAREN",
    "TOKTYPE_RPAREN", "TOKTYPE_LBRACE", "TOKTYPE_RBRACE", "TOKTYPE_LBRACKET",
    "TOKTYPE_RBRACKET", "TOKTYPE_SEMICOLON", "TOKTYPE_COMMA", "TOKTYPE_DOT",
    "TOKTYPE_ARROW", "TOKTYPE_QUESTION", "TOKTYPE_COLON", "TOKTYPE_PLUS",
    "TOKTYPE_MINUS", "TOKTYPE_STAR", "TOKTYPE_SLASH", "TOKTYPE_PERCENT",
    "TOKTYPE_PLUS_PLUS", "TOKTYPE_MINUS_MINUS", "TOKTYPE_EQUAL", "TOKTYPE_MUL_ASSIGN",
    "TOKTYPE_DIV_ASSIGN", "TOKTYPE_MOD_ASSIGN", "TOKTYPE_ADD_ASSIGN",
    "TOKTYPE_SUB_ASSIGN", "TOKTYPE_LEFT_ASSIGN", "TOKTYPE_RIGHT_ASSIGN",
    "TOKTYPE_AND_ASSIGN", "TOKTYPE_XOR_ASSIGN", "TOKTYPE_OR_ASSIGN", "TOKTYPE_EQUAL_EQUAL",
    "TOKTYPE_NOT_EQUAL", "TOKTYPE_LESS", "TOKTYPE_GREATER", "TOKTYPE_LESS_EQUAL",
    "TOKTYPE_GREATER_EQUAL", "TOKTYPE_AND_AND", "TOKTYPE_OR_OR", "TOKTYPE_BANG", "TOKTYPE_AMPERSAND",
    "TOKTYPE_PIPE", "TOKTYPE_CARET", "TOKTYPE_TILDE", "TOKTYPE_SHIFT_LEFT", "TOKTYPE_SHIFT_RIGHT",
    "TOKTYPE_ELLIPSIS"};

const char *token_type_error_names[] = {
    "EOF", "INVALID", "identifier", "integer literal", "'int'", "'void'", "'return'", "'if'",
    "'else'", "'while'", "'for'", "(", ")", "{", "}", "[", "]", ";", ",", ".", "->", "?", ":",
    "+", "-", "*", "/", "%", "++", "--", "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=",
    "^=", "|=", "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "..."};

static void populate_token(token_type_t type, char *text, token_t *out_token)
{
    ASSERT(out_token);
    out_token->type = type;
    out_token->text = text;
}

#define INITIAL_IDENTIFIER_SIZE 16

size_t g_lex_index, g_lex_col, g_lex_row;

static char *lex_identifier(const char *text)
{
    ASSERT(isalpha(text[g_lex_index]) || text[g_lex_index] == '_');

    char *identifier = malloc(INITIAL_IDENTIFIER_SIZE);
    if (!identifier)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    size_t identifier_capacity = INITIAL_IDENTIFIER_SIZE;
    size_t identifier_size = 0;

    while (text[g_lex_index] && (isalpha(text[g_lex_index]) || isdigit(text[g_lex_index]) || text[g_lex_index] == '_'))
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

#define INITIAL_INTLIT_SIZE 65

static char *lex_intlit(const char *text)
{
    ASSERT(isdigit(text[g_lex_index]));

    char *intlit = malloc(INITIAL_INTLIT_SIZE);
    if (!intlit)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    size_t intlit_capacity = INITIAL_INTLIT_SIZE;
    size_t intlit_size = 0;

    while (text[g_lex_index] && isdigit(text[g_lex_index]))
    {
        if (intlit_size >= (intlit_capacity - 1))
        {
            intlit_capacity *= 2;
            intlit = realloc(intlit, intlit_capacity);
            if (!intlit)
            {
                log_error("failed to allocate memory");
                exit(EXIT_FAILURE);
            }
        }

        intlit[intlit_size++] = text[g_lex_index++];
        g_lex_col++;
    }

    intlit[intlit_size] = 0;
    return intlit;
}

void init_lexer(void)
{
    g_lex_index = 0;
    g_lex_col = 0;
    g_lex_row = 0;
}

#define LEXER_ADD_SINGLE_CASE(toktype, c)    \
    else if (text[g_lex_index] == c[0])      \
    {                                        \
        populate_token(toktype, c, out_tok); \
        out_tok->start.col = g_lex_col;      \
        out_tok->start.row = g_lex_row;      \
                                             \
        g_lex_index++;                       \
        g_lex_col++;                         \
                                             \
        out_tok->end.col = g_lex_col;        \
        out_tok->end.row = g_lex_row;        \
        return 1;                            \
    }

#define LEXER_ADD_DOUBLE_CASE(toktype1, c1, toktype2, c2)        \
    else if (text[g_lex_index] == c1[0])                         \
    {                                                            \
        position_t start = {.col = g_lex_col, .row = g_lex_row}; \
                                                                 \
        g_lex_index++;                                           \
        g_lex_col++;                                             \
                                                                 \
        if (text[g_lex_index] && text[g_lex_index] == c2[1])     \
        {                                                        \
            g_lex_index++;                                       \
            g_lex_col++;                                         \
                                                                 \
            populate_token(toktype2, c2, out_tok);               \
            out_tok->start = start;                              \
                                                                 \
            out_tok->end.col = g_lex_col;                        \
            out_tok->end.row = g_lex_row;                        \
        }                                                        \
        else                                                     \
        {                                                        \
            populate_token(toktype1, c1, out_tok);               \
            out_tok->start = start;                              \
                                                                 \
            out_tok->end.col = g_lex_col;                        \
            out_tok->end.row = g_lex_row;                        \
        }                                                        \
        return 1;                                                \
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
        else if (isalpha(text[g_lex_index]) || text[g_lex_index] == '_')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};
            char *identifier = lex_identifier(text);

            token_type_t type = TOKTYPE_IDENTIFIER;
            if (strcmp(identifier, "int") == 0)
                type = TOKTYPE_KW_INT;
            else if (strcmp(identifier, "void") == 0)
                type = TOKTYPE_KW_VOID;
            else if (strcmp(identifier, "return") == 0)
                type = TOKTYPE_KW_RETURN;
            else if (strcmp(identifier, "if") == 0)
                type = TOKTYPE_KW_IF;
            else if (strcmp(identifier, "else") == 0)
                type = TOKTYPE_KW_ELSE;
            else if (strcmp(identifier, "while") == 0)
                type = TOKTYPE_KW_WHILE;
            else if (strcmp(identifier, "for") == 0)
                type = TOKTYPE_KW_FOR;

            populate_token(type, identifier, out_tok);

            out_tok->start.col = start.col;
            out_tok->start.row = start.row;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;

            return 1;
        }
        else if (isdigit(text[g_lex_index]))
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};
            char *intlit = lex_intlit(text);

            populate_token(TOKTYPE_INTLIT, intlit, out_tok);

            out_tok->start.col = start.col;
            out_tok->start.row = start.row;

            out_tok->end.col = g_lex_col;
            out_tok->end.row = g_lex_row;

            return 1;
        }
        else if (text[g_lex_index] == '-')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index] == '>')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_ARROW, "->", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '=')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_SUB_ASSIGN, "-=", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '-')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_MINUS_MINUS, "--", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else
            {
                populate_token(TOKTYPE_MINUS, "-", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }
        else if (text[g_lex_index] == '+')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index] == '=')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_ADD_ASSIGN, "+=", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '+')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_PLUS_PLUS, "++", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else
            {
                populate_token(TOKTYPE_PLUS, "+", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }
        else if (text[g_lex_index] == '<')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index] == '=')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_LESS_EQUAL, "<=", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '<')
            {
                g_lex_index++;
                g_lex_col++;

                if (text[g_lex_index] && text[g_lex_index] == '=')
                {
                    g_lex_index++;
                    g_lex_col++;

                    populate_token(TOKTYPE_LEFT_ASSIGN, "<<=", out_tok);
                    out_tok->start = start;

                    out_tok->end.col = g_lex_col;
                    out_tok->end.row = g_lex_row;
                }
                else
                {
                    populate_token(TOKTYPE_SHIFT_LEFT, "<<", out_tok);
                    out_tok->start = start;

                    out_tok->end.col = g_lex_col;
                    out_tok->end.row = g_lex_row;
                }
            }
            else
            {
                populate_token(TOKTYPE_LESS, "<", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }
        else if (text[g_lex_index] == '>')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index] == '=')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_GREATER_EQUAL, ">=", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '>')
            {
                g_lex_index++;
                g_lex_col++;

                if (text[g_lex_index] && text[g_lex_index] == '=')
                {
                    g_lex_index++;
                    g_lex_col++;

                    populate_token(TOKTYPE_RIGHT_ASSIGN, ">>=", out_tok);
                    out_tok->start = start;

                    out_tok->end.col = g_lex_col;
                    out_tok->end.row = g_lex_row;
                }
                else
                {
                    populate_token(TOKTYPE_SHIFT_RIGHT, ">>", out_tok);
                    out_tok->start = start;

                    out_tok->end.col = g_lex_col;
                    out_tok->end.row = g_lex_row;
                }
            }
            else
            {
                populate_token(TOKTYPE_GREATER, ">", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }
        else if (text[g_lex_index] == '&')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index] == '=')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_AND_ASSIGN, "&=", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '&')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_AND_AND, "&&", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else
            {
                populate_token(TOKTYPE_AMPERSAND, "&", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }
        else if (text[g_lex_index] == '|')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index] == '=')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_OR_ASSIGN, "|=", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else if (text[g_lex_index] && text[g_lex_index] == '|')
            {
                g_lex_index++;
                g_lex_col++;

                populate_token(TOKTYPE_OR_OR, "||", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else
            {
                populate_token(TOKTYPE_PIPE, "|", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }
        else if (text[g_lex_index] == '.')
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};

            g_lex_index++;
            g_lex_col++;

            if (text[g_lex_index] && text[g_lex_index + 1] && text[g_lex_index] == '.' && text[g_lex_index + 1] == '.')
            {
                g_lex_index += 2;
                g_lex_col += 2;

                populate_token(TOKTYPE_ELLIPSIS, "...", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            else
            {
                populate_token(TOKTYPE_DOT, ".", out_tok);
                out_tok->start = start;

                out_tok->end.col = g_lex_col;
                out_tok->end.row = g_lex_row;
            }
            return 1;
        }

        LEXER_ADD_SINGLE_CASE(TOKTYPE_LPAREN, "(")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_RPAREN, ")")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_LBRACE, "{")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_RBRACE, "}")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_LBRACKET, "[")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_RBRACKET, "]")

        LEXER_ADD_SINGLE_CASE(TOKTYPE_SEMICOLON, ";")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_COMMA, ",")

        LEXER_ADD_SINGLE_CASE(TOKTYPE_QUESTION, "?")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_COLON, ":")
        LEXER_ADD_SINGLE_CASE(TOKTYPE_TILDE, "~")

        LEXER_ADD_DOUBLE_CASE(TOKTYPE_STAR, "*", TOKTYPE_MUL_ASSIGN, "*=")
        LEXER_ADD_DOUBLE_CASE(TOKTYPE_SLASH, "/", TOKTYPE_DIV_ASSIGN, "/=")
        LEXER_ADD_DOUBLE_CASE(TOKTYPE_PERCENT, "%", TOKTYPE_MOD_ASSIGN, "%=")
        LEXER_ADD_DOUBLE_CASE(TOKTYPE_CARET, "^", TOKTYPE_XOR_ASSIGN, "^=")
        LEXER_ADD_DOUBLE_CASE(TOKTYPE_EQUAL, "=", TOKTYPE_EQUAL_EQUAL, "==")
        LEXER_ADD_DOUBLE_CASE(TOKTYPE_BANG, "!", TOKTYPE_NOT_EQUAL, "!=")

        else
        {
            position_t start = {.col = g_lex_col, .row = g_lex_row};
            g_lex_index++;
            g_lex_col++;
            position_t end = {.col = g_lex_col, .row = g_lex_row};

            populate_token(TOKTYPE_INVALID, NULL, out_tok);
            out_tok->start = start;
            out_tok->end = end;

            push_error(&start, &end, "unrecognized character");
            return 1;
        }
    }

    populate_token(TOKTYPE_EOF, NULL, out_tok);
    return 0;
}
