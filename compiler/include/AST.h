#pragma once

#include "tokstream.h"
#include "array.h"

typedef enum
{
    AST_NODE_TYPE_TRANSLATION_UNIT,
    AST_NODE_TYPE_FUNCTION_DEFINITION,
    AST_NODE_TYPE_BUILTIN_TYPE,
    AST_NODE_TYPE_COMPOUND_STATEMENT
} AST_node_type_t;

typedef struct _AST_node
{
    AST_node_type_t type;
    token_t *tok;

    position_t start;
    position_t end;

    array_t(struct _AST_node *) children;
} AST_node_t;
