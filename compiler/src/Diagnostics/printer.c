#include "parser.h"
#include "logger.h"

#include <stdio.h>
#include <stdbool.h>

static const char *AST_node_type_to_string(AST_node_type_t type)
{
    switch (type)
    {
    case AST_NODE_TYPE_TRANSLATION_UNIT:
        return "TRANSLATION_UNIT";

    case AST_NODE_TYPE_FUNCTION_DEFINITION:
        return "FUNCTION_DEFINITION";
    case AST_NODE_TYPE_DECLARATION:
        return "DECLARATION";
    case AST_NODE_TYPE_PARAMETER:
        return "PARAMETER";

    case AST_NODE_TYPE_INIT_DECLARATOR:
        return "AST_NODE_TYPE_INIT_DECLARATOR";
    case AST_NODE_TYPE_BUILTIN_TYPE:
        return "BUILTIN_TYPE";
    case AST_NODE_TYPE_DECLARATOR:
        return "DECLARATOR";
    case AST_NODE_TYPE_POINTER_TYPE:
        return "POINTER_TYPE";
    case AST_NODE_TYPE_FUNCTION_TYPE:
        return "FUNCTION_TYPE";
    case AST_NODE_TYPE_ARRAY_TYPE:
        return "ARRAY_TYPE";

    case AST_NODE_TYPE_COMPOUND_STATEMENT:
        return "COMPOUND_STATEMENT";
    case AST_NODE_TYPE_EXPRESSION_STATEMENT:
        return "EXPRESSION_STATEMENT";
    case AST_NODE_TYPE_IF_STATEMENT:
        return "IF_STATEMENT";
    case AST_NODE_TYPE_WHILE_STATEMENT:
        return "WHILE_STATEMENT";
    case AST_NODE_TYPE_FOR_STATEMENT:
        return "FOR_STATEMENT";
    case AST_NODE_TYPE_RETURN_STATEMENT:
        return "RETURN_STATEMENT";

    case AST_NODE_TYPE_IDENTIFIER:
        return "IDENTIFIER";
    case AST_NODE_TYPE_INTEGER_LITERAL:
        return "INTEGER_LITERAL";

    case AST_NODE_TYPE_UNARY_OPERATION:
        return "UNARY_OPERATION";
    case AST_NODE_TYPE_BINARY_OPERATION:
        return "BINARY_OPERATION";
    case AST_NODE_TYPE_ASSIGNMENT:
        return "ASSIGNMENT";
    case AST_NODE_TYPE_CONDITIONAL_OPERATION:
        return "CONDITIONAL_OPERATION";

    case AST_NODE_TYPE_FUNCTION_CALL:
        return "FUNCTION_CALL";
    case AST_NODE_TYPE_ARRAY_SUBSCRIPT:
        return "ARRAY_SUBSCRIPT";
    case AST_NODE_TYPE_MEMBER_ACCESS:
        return "MEMBER_ACCESS";
    case AST_NODE_TYPE_POINTER_MEMBER_ACCESS:
        return "POINTER_MEMBER_ACCESS";
    case AST_NODE_TYPE_POSTFIX_OPERATION:
        return "POSTFIX_OPERATION";

    default:
        return "UNKNOWN";
    }
}

static void print_indent(const bool *has_sibling, size_t depth)
{
    if (depth == 0)
        return;

    for (size_t i = 0; i + 1 < depth; ++i)
        printf("%s", has_sibling[i] ? "│   " : "    ");

    printf("%s", has_sibling[depth - 1] ? "├── " : "└── ");
}

static void print_tokens(const AST_node_t *node)
{
    size_t count = array_size(node->tokens);

    if (count == 0)
        return;

    printf(" [");

    for (size_t i = 0; i < count; ++i)
    {
        if (i != 0)
            printf(", ");

        token_t *tok = node->tokens[i];
        printf("%s", tok->text);
    }

    printf("]");
}

static void print_ast_node(const AST_node_t *node, bool *has_sibling, size_t depth)
{
    if (!node)
    {
        print_indent(has_sibling, depth);
        printf("NULL\n");
        return;
    }

    print_indent(has_sibling, depth);

    printf("%s", AST_node_type_to_string(node->type));
    print_tokens(node);

    printf("\n");

    size_t child_count = array_size(node->children);

    for (size_t i = 0; i < child_count; ++i)
    {
        has_sibling[depth] = i + 1 < child_count;
        print_ast_node(node->children[i], has_sibling, depth + 1);
    }
}

void print_AST(const AST_node_t *root)
{
    if (!root)
    {
        printf("(null AST)\n");
        return;
    }

    bool has_sibling[256] = {0};
    print_ast_node(root, has_sibling, 0);
}
