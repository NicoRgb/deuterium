#include "printer.h"

static const char *AST_node_type_to_string(AST_node_type_t type)
{
    switch (type)
    {
    case AST_NODE_TYPE_TRANSLATION_UNIT:
        return "TRANSLATION_UNIT";
    case AST_NODE_TYPE_FUNCTION_DEFINITION:
        return "FUNCTION_DEFINITION";
    case AST_NODE_TYPE_BUILTIN_TYPE:
        return "BUILTIN_TYPE";
    case AST_NODE_TYPE_COMPOUND_STATEMENT:
        return "COMPOUND_STATEMENT";
    default:
        return "ERROR TYPE";
    }
}

#define AST_NODE_INDENT 2

static void print_indent(uint16_t indent)
{
    for (uint16_t i = 0; i < indent; i++)
        putc(' ', stdout);
}

static void print_AST_node(AST_node_t *node, uint16_t indent)
{
    print_indent(indent);
    printf("%s\n", AST_node_type_to_string(node->type));

    indent += AST_NODE_INDENT;

    if (node->tok)
    {
        print_indent(indent);
        print_token(node->tok);
    }

    for (size_t i = 0; i < array_size(node->children); i++)
    {
        print_AST_node(node->children[i], indent);
    }
}

void print_token(token_t *token)
{
    if (token->type == TOKTYPE_KEYWORD || token->type == TOKTYPE_IDENTIFIER)
        printf("%s(%s)\n", token_type_names[token->type], token->text);
    else
        printf("%s", token_type_names[token->type]);
}

void print_AST(AST_node_t *AST)
{
    if (AST)
        print_AST_node(AST, 0);
}
