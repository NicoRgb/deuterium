#include "AST.h"
#include "scope.h"
#include "strdup.h"

static AST_node_t *desugar_statement(AST_node_t *node);

static AST_node_t *desugar_for_statement(AST_node_t *node)
{
    ASSERT(node);

    AST_node_t *compound = create_AST_node(AST_NODE_TYPE_COMPOUND_STATEMENT);
    node_push_child(compound, node->children[0]);

    AST_node_t *condition = node->children[1];
    AST_node_t *while_stmt = create_AST_node(AST_NODE_TYPE_WHILE_STATEMENT);
    if (array_size(condition->children) > 0)
    {
        node_push_child(while_stmt, condition->children[0]);
        free_AST_node_layer(condition);
    }
    else
    {
        free_AST_node(condition);

        condition = create_AST_node(AST_NODE_TYPE_INTEGER_LITERAL);
        node_push_token(condition, tok_forge(TOKTYPE_INTLIT, strdup("1")));
        node_push_child(while_stmt, condition);
    }

    if (array_size(node->children) > 3)
    {
        AST_node_t *expr_stmt = create_AST_node(AST_NODE_TYPE_EXPRESSION_STATEMENT);
        node_push_child(expr_stmt, node->children[2]);

        AST_node_t *while_compound = create_AST_node(AST_NODE_TYPE_COMPOUND_STATEMENT);
        node_push_child(while_compound, node->children[3]);
        node_push_child(while_compound, expr_stmt);

        node_push_child(while_stmt, while_compound);
    }
    else
    {
        node_push_child(while_stmt, node->children[2]);
    }

    free_AST_node_layer(node);

    node_push_child(compound, while_stmt);
    return compound;
}

static AST_node_t *desugar_compound_statement(AST_node_t *node)
{
    ASSERT(node);

    AST_node_t *it;
    foreach (node->children, it)
    {
        switch (it->type)
        {
        case AST_NODE_TYPE_DECLARATION:
            break;

        default:
            node->children[_i] = desugar_statement(it);
            break;
        }
    }

    return node;
}

static AST_node_t *desugar_statement(AST_node_t *node)
{
    ASSERT(node);

    switch (node->type)
    {
    case AST_NODE_TYPE_COMPOUND_STATEMENT:
        return desugar_compound_statement(node);

    case AST_NODE_TYPE_EXPRESSION_STATEMENT:
        return node;

    case AST_NODE_TYPE_IF_STATEMENT:
        return node;

    case AST_NODE_TYPE_WHILE_STATEMENT:
        return node;

    case AST_NODE_TYPE_FOR_STATEMENT:
        return desugar_for_statement(node);

    case AST_NODE_TYPE_RETURN_STATEMENT:
        return node;

    default:
        ASSERT(0);
    }
}

void desugar_typed_AST(AST_node_t *node)
{
    ASSERT(node);

    AST_node_t *it;
    foreach (node->children, it)
    {
        switch (it->type)
        {
        case AST_NODE_TYPE_DECLARATION:
            break;

        case AST_NODE_TYPE_FUNCTION_DEFINITION:
            node->children[_i] = desugar_compound_statement(it);
            break;

        default:
            ASSERT(0);
        }
    }
}
