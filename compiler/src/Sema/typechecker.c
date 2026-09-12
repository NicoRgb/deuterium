#include "sema.h"

typedef enum
{
    TYPERULE_RES_LEFT_COPY,
    TYPERULE_RES_RIGHT_COPY,
    TYPERULE_RES_POINTER_TO_LEFT,
    TYPERULE_RES_POINTER_TO_RIGHT,
    TYPERULE_RES_DEREFERENCE_LEFT,
    TYPERULE_RES_DEREFERENCE_RIGHT,

    TYPERULE_RES_VOID,
    TYPERULE_RES_INT,
    TYPERULE_RES_INTLIT,
} typerule_result_t;

typedef struct
{
    token_type_t op;
    type_kind_t kind1;
    type_kind_t kind2;
    typerule_result_t res;
    uint8_t unary;
} typerule_t;

#define BINARY_RULE(op, left, right, res) {(op), (left), (right), (res), 0}
#define UNARY_RULE(op, type, res) {(op), (type), (type), (res), 1}

static const typerule_t binary_rules[] = {
    /* assignment */
    BINARY_RULE(TOKTYPE_EQUAL, TYPE_INT, TYPE_INT, TYPERULE_RES_LEFT_COPY),
    BINARY_RULE(TOKTYPE_EQUAL, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_LEFT_COPY),
    BINARY_RULE(TOKTYPE_EQUAL, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_LEFT_COPY),

    /* comma */
    BINARY_RULE(TOKTYPE_COMMA, TYPE_INT, TYPE_INT, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_POINTER, TYPE_INT, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_POINTER, TYPE_INTLIT, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_INT, TYPE_POINTER, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_COMMA, TYPE_INTLIT, TYPE_POINTER, TYPERULE_RES_RIGHT_COPY),

    /* logical */
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_POINTER, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_POINTER, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_INT, TYPE_POINTER, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_OR_OR, TYPE_INTLIT, TYPE_POINTER, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_AND_AND, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_POINTER, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_POINTER, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_INT, TYPE_POINTER, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AND_AND, TYPE_INTLIT, TYPE_POINTER, TYPERULE_RES_INT),

    /* bitwise */
    BINARY_RULE(TOKTYPE_PIPE, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PIPE, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PIPE, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PIPE, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_CARET, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_CARET, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_CARET, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_CARET, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_AMPERSAND, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AMPERSAND, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AMPERSAND, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_AMPERSAND, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    /* equality */
    BINARY_RULE(TOKTYPE_EQUAL_EQUAL, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_EQUAL_EQUAL, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_EQUAL_EQUAL, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_EQUAL_EQUAL, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_EQUAL_EQUAL, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_NOT_EQUAL, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_NOT_EQUAL, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_NOT_EQUAL, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_NOT_EQUAL, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_NOT_EQUAL, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    /* relational */
    BINARY_RULE(TOKTYPE_LESS, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_GREATER, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_LESS_EQUAL, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS_EQUAL, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS_EQUAL, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS_EQUAL, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_LESS_EQUAL, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_GREATER_EQUAL, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER_EQUAL, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER_EQUAL, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER_EQUAL, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_GREATER_EQUAL, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    /* shifts */
    BINARY_RULE(TOKTYPE_SHIFT_LEFT, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SHIFT_LEFT, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SHIFT_LEFT, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SHIFT_LEFT, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_SHIFT_RIGHT, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SHIFT_RIGHT, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SHIFT_RIGHT, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SHIFT_RIGHT, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    /* arithmetic */
    BINARY_RULE(TOKTYPE_PLUS, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_POINTER, TYPE_INT, TYPERULE_RES_LEFT_COPY),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_POINTER, TYPE_INTLIT, TYPERULE_RES_LEFT_COPY),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_INT, TYPE_POINTER, TYPERULE_RES_RIGHT_COPY),
    BINARY_RULE(TOKTYPE_PLUS, TYPE_INTLIT, TYPE_POINTER, TYPERULE_RES_RIGHT_COPY),

    BINARY_RULE(TOKTYPE_MINUS, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_MINUS, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_MINUS, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_MINUS, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_MINUS, TYPE_POINTER, TYPE_INT, TYPERULE_RES_LEFT_COPY),
    BINARY_RULE(TOKTYPE_MINUS, TYPE_POINTER, TYPE_INTLIT, TYPERULE_RES_LEFT_COPY),
    BINARY_RULE(TOKTYPE_MINUS, TYPE_POINTER, TYPE_POINTER, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_STAR, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_STAR, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_STAR, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_STAR, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_SLASH, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SLASH, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SLASH, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_SLASH, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),

    BINARY_RULE(TOKTYPE_PERCENT, TYPE_INT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PERCENT, TYPE_INT, TYPE_INTLIT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PERCENT, TYPE_INTLIT, TYPE_INT, TYPERULE_RES_INT),
    BINARY_RULE(TOKTYPE_PERCENT, TYPE_INTLIT, TYPE_INTLIT, TYPERULE_RES_INT),
};

static const typerule_t unary_rules[] = {
    /* increment / decrement */
    UNARY_RULE(TOKTYPE_PLUS_PLUS, TYPE_INT, TYPERULE_RES_LEFT_COPY),
    UNARY_RULE(TOKTYPE_PLUS_PLUS, TYPE_INTLIT, TYPERULE_RES_LEFT_COPY),
    UNARY_RULE(TOKTYPE_PLUS_PLUS, TYPE_POINTER, TYPERULE_RES_LEFT_COPY),

    UNARY_RULE(TOKTYPE_MINUS_MINUS, TYPE_INT, TYPERULE_RES_LEFT_COPY),
    UNARY_RULE(TOKTYPE_MINUS_MINUS, TYPE_INTLIT, TYPERULE_RES_LEFT_COPY),
    UNARY_RULE(TOKTYPE_MINUS_MINUS, TYPE_POINTER, TYPERULE_RES_LEFT_COPY),

    /* address-of */
    UNARY_RULE(TOKTYPE_AMPERSAND, TYPE_INT, TYPERULE_RES_POINTER_TO_LEFT),
    UNARY_RULE(TOKTYPE_AMPERSAND, TYPE_INTLIT, TYPERULE_RES_POINTER_TO_LEFT),
    UNARY_RULE(TOKTYPE_AMPERSAND, TYPE_ARRAY, TYPERULE_RES_POINTER_TO_LEFT),
    UNARY_RULE(TOKTYPE_AMPERSAND, TYPE_POINTER, TYPERULE_RES_POINTER_TO_LEFT),
    UNARY_RULE(TOKTYPE_AMPERSAND, TYPE_FUNCTION, TYPERULE_RES_POINTER_TO_LEFT),

    /* dereference */
    UNARY_RULE(TOKTYPE_STAR, TYPE_POINTER, TYPERULE_RES_DEREFERENCE_LEFT),

    /* unary arithmetic */
    UNARY_RULE(TOKTYPE_PLUS, TYPE_INT, TYPERULE_RES_INT),
    UNARY_RULE(TOKTYPE_PLUS, TYPE_INTLIT, TYPERULE_RES_INTLIT),
    UNARY_RULE(TOKTYPE_MINUS, TYPE_INT, TYPERULE_RES_INT),
    UNARY_RULE(TOKTYPE_MINUS, TYPE_INTLIT, TYPERULE_RES_INTLIT),

    UNARY_RULE(TOKTYPE_TILDE, TYPE_INT, TYPERULE_RES_INT),
    UNARY_RULE(TOKTYPE_TILDE, TYPE_INTLIT, TYPERULE_RES_INT),

    UNARY_RULE(TOKTYPE_BANG, TYPE_INT, TYPERULE_RES_INT),
    UNARY_RULE(TOKTYPE_BANG, TYPE_INTLIT, TYPERULE_RES_INT),
    UNARY_RULE(TOKTYPE_BANG, TYPE_POINTER, TYPERULE_RES_INT),
};

static bool is_scalar_type(const type_t *type)
{
    return type->kind == TYPE_INT ||
           type->kind == TYPE_INTLIT ||
           type->kind == TYPE_POINTER;
}

static type_t *check_integer_literal(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_INTEGER_LITERAL);
    ASSERT(node->tokens[0]->type == TOKTYPE_INTLIT);

    type_t *res = create_type();
    res->kind = TYPE_INTLIT;

    return res;
}

static type_t *check_unary_operation(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_UNARY_OPERATION);

    type_t *type = check_expression(node->children[0]);
    if (!type)
        return NULL;

    for (size_t i = 0; i < sizeof(unary_rules) / sizeof(typerule_t); i++)
    {
        if (unary_rules[i].unary && unary_rules[i].op == node->tokens[0]->type && unary_rules[i].kind1 == type->kind)
        {
            switch (unary_rules[i].res)
            {
            case TYPERULE_RES_LEFT_COPY:
            case TYPERULE_RES_RIGHT_COPY:
                return type;

            case TYPERULE_RES_POINTER_TO_LEFT:
            case TYPERULE_RES_POINTER_TO_RIGHT:
            {
                type_t *res = create_type();
                res->kind = TYPE_POINTER;
                res->pointer.dest = type;
                return res;
            }

            case TYPERULE_RES_DEREFERENCE_LEFT:
            case TYPERULE_RES_DEREFERENCE_RIGHT:
            {
                type_t *dest = type->pointer.dest;
                free(type);
                return dest;
            }

            case TYPERULE_RES_VOID:
                free_type(type);
                type = create_type();
                type->kind = TYPE_VOID;
                return type;

            case TYPERULE_RES_INT:
                free_type(type);
                type = create_type();
                type->kind = TYPE_INT;
                return type;

            case TYPERULE_RES_INTLIT:
                free_type(type);
                type = create_type();
                type->kind = TYPE_INTLIT;
                return type;

            default:
                ASSERT(0);
            }
        }
    }

    push_error(&node->start, &node->end, "type error");
    free_type(type);
    return NULL;
}

static type_t *check_binary_operation(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_BINARY_OPERATION || node->type == AST_NODE_TYPE_ASSIGNMENT);

    type_t *left = NULL;
    type_t *right = NULL;

    left = check_expression(node->children[0]);
    if (!left)
        goto out;

    right = check_expression(node->children[1]);
    if (!right)
        goto out;

    for (size_t i = 0; i < sizeof(binary_rules) / sizeof(typerule_t); i++)
    {
        if (!binary_rules[i].unary && binary_rules[i].op == node->tokens[0]->type && binary_rules[i].kind1 == left->kind && binary_rules[i].kind2 == right->kind)
        {
            switch (binary_rules[i].res)
            {
            case TYPERULE_RES_LEFT_COPY:
                free_type(right);
                return left;

            case TYPERULE_RES_RIGHT_COPY:
                free_type(left);
                return right;

            case TYPERULE_RES_POINTER_TO_LEFT:
            {
                free_type(right);

                type_t *res = create_type();
                res->kind = TYPE_POINTER;
                res->pointer.dest = left;
                return res;
            }

            case TYPERULE_RES_POINTER_TO_RIGHT:
            {
                free_type(left);

                type_t *res = create_type();
                res->kind = TYPE_POINTER;
                res->pointer.dest = right;
                return res;
            }

            case TYPERULE_RES_DEREFERENCE_LEFT:
            {
                free_type(right);

                type_t *dest = left->pointer.dest;
                free(left);
                return dest;
            }

            case TYPERULE_RES_DEREFERENCE_RIGHT:
            {
                free_type(left);

                type_t *dest = right->pointer.dest;
                free(right);
                return dest;
            }

            case TYPERULE_RES_VOID:
                free_type(left);
                free_type(right);
                left = create_type();
                left->kind = TYPE_VOID;
                return left;

            case TYPERULE_RES_INT:
                free_type(left);
                free_type(right);
                left = create_type();
                left->kind = TYPE_INT;
                return left;

            case TYPERULE_RES_INTLIT:
                free_type(left);
                free_type(right);
                left = create_type();
                left->kind = TYPE_INTLIT;
                return left;

            default:
                ASSERT(0);
            }
        }
    }

out:
    push_error(&node->start, &node->end, "type error");
    if (left)
        free_type(left);

    if (right)
        free_type(right);

    return NULL;
}

static type_t *check_assignment(AST_node_t *node)
{
    return check_binary_operation(node); // can be handled by binary operation
}

static type_t *check_conditional_operation(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_CONDITIONAL_OPERATION);

    type_t *condition = NULL;
    type_t *child1 = NULL;
    type_t *child2 = NULL;

    condition = check_expression(node->children[0]);
    if (!condition || !is_scalar_type(condition))
    {
        goto out;
    }

    child1 = check_expression(node->children[1]);
    if (!child1)
    {
        goto out;
    }

    child2 = check_expression(node->children[2]);
    if (!child2)
    {
        goto out;
    }

    if (child1->kind == TYPE_INT && child2->kind == TYPE_INT)
    {
        free_type(condition);
        free_type(child2);
        return child1;
    }

    if (child1->kind == TYPE_INT && child2->kind == TYPE_INTLIT)
    {
        free_type(condition);
        free_type(child2);
        return child1;
    }

    if (child1->kind == TYPE_INTLIT && child2->kind == TYPE_INT)
    {
        free_type(condition);
        free_type(child1);
        return child2;
    }

    if (child1->kind == TYPE_INTLIT && child2->kind == TYPE_INTLIT)
    {
        free_type(condition);
        free_type(child2);
        return child1;
    }

    if (child1->kind == TYPE_POINTER && child2->kind == TYPE_POINTER && compare_types(child1, child2))
    {
        free_type(condition);
        free_type(child2);
        return child1;
    }

out:
    push_error(&node->start, &node->end, "type error");
    if (condition)
        free_type(condition);

    if (child1)
        free_type(child1);

    if (child2)
        free_type(child2);

    return NULL;
}

static type_t *check_identifier(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->tokens[0]->type == TOKTYPE_IDENTIFIER);
    symbol_t *sym = resolve_symbol(node->tokens[0]->text);
    if (!sym)
        return NULL;

    return clone_type(sym->type);
}

type_t *check_expression(AST_node_t *node)
{
    ASSERT(node);
    switch (node->type)
    {
    case AST_NODE_TYPE_IDENTIFIER:
        return check_identifier(node);
        break;

    case AST_NODE_TYPE_INTEGER_LITERAL:
        return check_integer_literal(node);

    case AST_NODE_TYPE_UNARY_OPERATION:
        return check_unary_operation(node);

    case AST_NODE_TYPE_BINARY_OPERATION:
        return check_binary_operation(node);

    case AST_NODE_TYPE_CONDITIONAL_OPERATION:
        return check_conditional_operation(node);

    case AST_NODE_TYPE_ASSIGNMENT:
        return check_assignment(node);
        break;

    default:
        ASSERT(false);
        break;
    }

    return NULL;
}
