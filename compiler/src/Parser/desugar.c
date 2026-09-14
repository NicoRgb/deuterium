#include "AST.h"
#include "scope.h"
#include "strutils.h"

static AST_node_t *desugar_expression(AST_node_t *node);
static AST_node_t *desugar_statement(AST_node_t *node);

static AST_node_t *desugar_unary_operation(AST_node_t *node)
{
    ASSERT(node);
    node->children[0] = desugar_expression(node->children[0]);

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_PLUS:
    {
        AST_node_t *res = node->children[0];
        free_AST_node_layer(node);
        return res;
    }
    case TOKTYPE_MINUS:
    {
        if (node->children[0]->type != AST_NODE_TYPE_INTEGER_LITERAL)
        {
            node->type = AST_NODE_TYPE_BINARY_OPERATION;
            node->tokens[0]->type = TOKTYPE_STAR;
            node->tokens[0]->text = "*";

            AST_node_t *intlit = create_AST_node(AST_NODE_TYPE_INTEGER_LITERAL);
            node_push_token(intlit, tok_forge(TOKTYPE_INTLIT, strdup("-1")));

            node_push_child(node, intlit);

            return node;
        }

        AST_node_t *res = node->children[0];
        char *text = res->tokens[0]->text;
        if (text[0] == '-')
        {
            size_t len = strlen(text);
            if (len > 0)
                memmove(text, text + 1, len);
        }
        else
        {
            res->tokens[0]->text = strcat_alloc("-", text);
            free(text);
        }

        free_AST_node_layer(node);
        return res;
    }
    case TOKTYPE_PLUS_PLUS:
    case TOKTYPE_MINUS_MINUS:
    {
        AST_node_t *binop = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        if (node->tokens[0]->type == TOKTYPE_PLUS_PLUS)
            node_push_token(binop, tok_forge(TOKTYPE_PLUS, "+"));
        else
            node_push_token(binop, tok_forge(TOKTYPE_MINUS, "-"));

        node->type = AST_NODE_TYPE_ASSIGNMENT;
        node->tokens[0] = tok_forge(TOKTYPE_EQUAL, "=");

        node_push_child(binop, node_clone(node->children[0]));

        AST_node_t *intlit = create_AST_node(AST_NODE_TYPE_INTEGER_LITERAL);
        node_push_token(intlit, tok_forge(TOKTYPE_INTLIT, "1"));

        node_push_child(binop, intlit);

        node_push_child(node, binop);

        return node;
    }
    default:
        break;
    }

    return node;
}

static AST_node_t *desugar_expression(AST_node_t *node)
{
    ASSERT(node);
    switch (node->type)
    {
    case AST_NODE_TYPE_BINARY_OPERATION:
        node->children[0] = desugar_expression(node->children[0]);
        node->children[1] = desugar_expression(node->children[1]);
        break;

    case AST_NODE_TYPE_UNARY_OPERATION:
        return desugar_unary_operation(node);

    case AST_NODE_TYPE_INTEGER_LITERAL:
        break;

    case AST_NODE_TYPE_IDENTIFIER:
        break;

    case AST_NODE_TYPE_POSTFIX_OPERATION:
        node->children[0] = desugar_expression(node->children[0]);
        break;

    case AST_NODE_TYPE_ASSIGNMENT:
        node->children[0] = desugar_expression(node->children[0]);
        node->children[1] = desugar_expression(node->children[1]);
        break;

    default:
        ASSERT(0);
    }

    return node;
}

static AST_node_t *desugar_for_statement(AST_node_t *node)
{
    ASSERT(node);

    switch (node->children[0]->type)
    {
    case AST_NODE_TYPE_EXPRESSION_STATEMENT:
        node->children[0] = desugar_statement(node->children[0]);

    case AST_NODE_TYPE_DECLARATION:
        break;

    default:
        ASSERT(0);
    }

    node->children[1] = desugar_statement(node->children[1]);

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
        node->children[2] = desugar_expression(node->children[2]);

        AST_node_t *expr_stmt = create_AST_node(AST_NODE_TYPE_EXPRESSION_STATEMENT);
        node_push_child(expr_stmt, node->children[2]);

        AST_node_t *while_compound = create_AST_node(AST_NODE_TYPE_COMPOUND_STATEMENT);

        node->children[3] = desugar_statement(node->children[3]);
        node_push_child(while_compound, node->children[3]);
        node_push_child(while_compound, expr_stmt);

        node_push_child(while_stmt, while_compound);
    }
    else
    {
        node->children[2] = desugar_statement(node->children[2]);
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
        node->children[0] = desugar_expression(node->children[0]);
        return node;

    case AST_NODE_TYPE_IF_STATEMENT:
    {
        node->children[0] = desugar_expression(node->children[0]);

        node->children[1] = desugar_statement(node->children[1]);
        if (array_size(node->children) > 2)
            node->children[2] = desugar_statement(node->children[2]);

        return node;
    }

    case AST_NODE_TYPE_WHILE_STATEMENT:
        node->children[0] = desugar_expression(node->children[0]);
        node->children[1] = desugar_statement(node->children[1]);
        return node;

    case AST_NODE_TYPE_FOR_STATEMENT:
        return desugar_for_statement(node);

    case AST_NODE_TYPE_RETURN_STATEMENT:
        if (array_size(node->children) > 0)
            node->children[0] = desugar_expression(node->children[0]);
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
