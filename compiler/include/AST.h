#pragma once

#include "tokstream.h"
#include "array.h"

typedef enum
{
    AST_NODE_TYPE_TRANSLATION_UNIT,

    AST_NODE_TYPE_FUNCTION_DEFINITION,
    AST_NODE_TYPE_DECLARATION,
    AST_NODE_TYPE_PARAMETER,

    AST_NODE_TYPE_INIT_DECLARATOR,
    AST_NODE_TYPE_BUILTIN_TYPE,
    AST_NODE_TYPE_DECLARATOR,
    AST_NODE_TYPE_POINTER_TYPE,
    AST_NODE_TYPE_FUNCTION_TYPE,
    AST_NODE_TYPE_ARRAY_TYPE,

    AST_NODE_TYPE_COMPOUND_STATEMENT,
    AST_NODE_TYPE_EXPRESSION_STATEMENT,
    AST_NODE_TYPE_IF_STATEMENT,
    AST_NODE_TYPE_WHILE_STATEMENT,
    AST_NODE_TYPE_FOR_STATEMENT,
    AST_NODE_TYPE_RETURN_STATEMENT,

    AST_NODE_TYPE_IDENTIFIER,
    AST_NODE_TYPE_INTEGER_LITERAL,

    AST_NODE_TYPE_UNARY_OPERATION,
    AST_NODE_TYPE_BINARY_OPERATION,
    AST_NODE_TYPE_ASSIGNMENT,
    AST_NODE_TYPE_CONDITIONAL_OPERATION,

    AST_NODE_TYPE_FUNCTION_CALL,
    AST_NODE_TYPE_ARRAY_SUBSCRIPT,
    AST_NODE_TYPE_MEMBER_ACCESS,
    AST_NODE_TYPE_POINTER_MEMBER_ACCESS,
    AST_NODE_TYPE_POSTFIX_OPERATION
} AST_node_type_t;

typedef struct _AST_node
{
    AST_node_type_t type;

    position_t start;
    position_t end;

    array_t(token_t *) tokens;
    array_t(struct _AST_node *) children;

    struct _symbol *symbol; // typed AST modified by sema stage
} AST_node_t;

void free_AST_node(AST_node_t *node);
void free_AST_node_layer(AST_node_t *node);
AST_node_t *create_AST_node(AST_node_type_t type);
void node_push_child(AST_node_t *node, AST_node_t *child);
void node_push_token(AST_node_t *node, token_t *tok);
AST_node_t *node_clone(AST_node_t *node);

void desugar_typed_AST(AST_node_t *node); // NOTE: source mappings are not preserved
