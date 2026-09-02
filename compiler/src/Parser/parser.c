#include "parser.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

static AST_node_t *create_AST_node(AST_node_type_t type)
{
    AST_node_t *node = malloc(sizeof(AST_node_t));
    if (!node)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    node->type = type;
    node->tok = NULL;
    array_create(AST_node_t *, node->children);

    return node;
}

static void node_push_child(AST_node_t *node, AST_node_t *child)
{
    ASSERT(node);
    array_push(node->children, child);
}

static void node_token(AST_node_t *node, token_t *tok)
{
    ASSERT(node);
    node->tok = tok;
}

AST_node_t *translation_unit(void);
AST_node_t *function_definition(void);
AST_node_t *type_specifier(void);
AST_node_t *parameter_list(void);
AST_node_t *compound_statement(void);

AST_node_t *parse(void)
{
    return translation_unit();
}

AST_node_t *translation_unit(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_TRANSLATION_UNIT);
    node_push_child(node, function_definition());
    return node;
}

AST_node_t *function_definition(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_FUNCTION_DEFINITION);
    node_push_child(node, type_specifier());

    token_t *identifier = tok_next();
    if (identifier->type != TOKTYPE_IDENTIFIER)
    {
        // TODO: error
    }

    node_token(node, identifier);

    tok_expect(TOKTYPE_LPAREN);
    parameter_list();
    tok_expect(TOKTYPE_RPAREN);

    compound_statement();
    return node;
}

AST_node_t *type_specifier(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_BUILTIN_TYPE);

    token_t *type = tok_next();
    if (type != TOKTYPE_KEYWORD || strcmp(type->text, "int") != 0)
    {
        // TODO: error
    }

    node_token(node, type);
    return node;
}

AST_node_t *parameter_list(void)
{
    token_t *type = tok_next();
    if (type != TOKTYPE_KEYWORD || strcmp(type->text, "void") != 0)
    {
        // TODO: error
    }
}

AST_node_t *compound_statement(void)
{
    tok_expect(TOKTYPE_LBRACE);
    tok_expect(TOKTYPE_RBRACE);
}
