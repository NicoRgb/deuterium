#include "sema.h"

static symbol_const_value_t eval_constant_integer_literal(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_INTEGER_LITERAL);
    ASSERT(node->tokens[0]->type == TOKTYPE_INTLIT);

    symbol_const_value_t result;
    result.is_valid = true;
    result.val = atoll(node->tokens[0]->text); // TODO: some real parsing
    result.type = create_type();
    result.type->kind = TYPE_INTLIT;

    return result;
}

static symbol_const_value_t eval_constant_unary_operation(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_UNARY_OPERATION);

    symbol_const_value_t result = eval_constant_expression(node->children[0]);
    if (!result.is_valid)
        return result;

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_PLUS_PLUS:
        result.val++;
        break;

    case TOKTYPE_MINUS_MINUS:
        result.val--;
        break;

    case TOKTYPE_AMPERSAND:
        result.is_valid = false;
        break;

    case TOKTYPE_STAR:
        result.is_valid = false;
        break;

    case TOKTYPE_PLUS:
        break;

    case TOKTYPE_MINUS:
        result.val *= -1;
        break;

    case TOKTYPE_TILDE:
        result.val = ~result.val;
        break;

    case TOKTYPE_BANG:
        result.val = !result.val;
        break;

    default:
        ASSERT(false);
    }

    return result;
}

static symbol_const_value_t eval_constant_binary_operation(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_BINARY_OPERATION);

    symbol_const_value_t left = eval_constant_expression(node->children[0]);
    if (!left.is_valid)
        return left;

    symbol_const_value_t right = eval_constant_expression(node->children[1]);
    if (!right.is_valid)
    {
        free_type(left.type);
        return right;
    }

    symbol_const_value_t result;
    result.is_valid = true;
    if (!compare_types(left.type, right.type)) // TODO: check type compatibility for individual cases
    {
        free_type(left.type);
        free_type(right.type);

        push_error(&node->start, &node->end, "invalid type conversion");
        result.is_valid = false;
        return result;
    }

    result.type = right.type;

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_EQUAL:
        free_type(left.type);
        return right;

    case TOKTYPE_COMMA:
        free_type(left.type);
        return right;

    case TOKTYPE_OR_OR:
        result.val = left.val || right.val;
        break;

    case TOKTYPE_AND_AND:
        result.val = left.val && right.val;
        break;

    case TOKTYPE_PIPE:
        result.val = left.val | right.val;
        break;

    case TOKTYPE_CARET:
        break;

    case TOKTYPE_AMPERSAND:
        result.val = left.val & right.val;
        break;

    case TOKTYPE_EQUAL_EQUAL:
        result.val = left.val == right.val;
        break;

    case TOKTYPE_NOT_EQUAL:
        result.val = left.val != right.val;
        break;

    case TOKTYPE_LESS:
        result.val = left.val < right.val;
        break;

    case TOKTYPE_GREATER:
        result.val = left.val > right.val;
        break;

    case TOKTYPE_LESS_EQUAL:
        result.val = left.val <= right.val;
        break;

    case TOKTYPE_GREATER_EQUAL:
        result.val = left.val >= right.val;
        break;

    case TOKTYPE_SHIFT_LEFT:
        result.val = left.val << right.val;
        break;

    case TOKTYPE_SHIFT_RIGHT:
        result.val = left.val >> right.val;
        break;

    case TOKTYPE_PLUS:
        result.val = left.val + right.val;
        break;

    case TOKTYPE_MINUS:
        result.val = left.val - right.val;
        break;

    case TOKTYPE_STAR:
        result.val = left.val * right.val;
        break;

    case TOKTYPE_SLASH:
        result.val = left.val / right.val;
        break;

    case TOKTYPE_PERCENT:
        result.val = left.val % right.val;
        break;

    default:
        ASSERT(false);
    }

    free_type(left.type);
    return result; // inherited right.type
}

static symbol_const_value_t eval_constant_conditional_operation(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_CONDITIONAL_OPERATION);

    symbol_const_value_t condition = eval_constant_expression(node->children[0]);
    if (!condition.is_valid)
        return condition;

    uint8_t child_index = condition.val ? 1 : 2;

    return eval_constant_expression(node->children[child_index]);
}

symbol_const_value_t eval_constant_expression(AST_node_t *node)
{
    symbol_const_value_t result;

    ASSERT(node);
    switch (node->type)
    {
    case AST_NODE_TYPE_IDENTIFIER:
        result.is_valid = false;
        break;

    case AST_NODE_TYPE_INTEGER_LITERAL:
        result = eval_constant_integer_literal(node);
        break;

    case AST_NODE_TYPE_UNARY_OPERATION:
        result = eval_constant_unary_operation(node);
        break;

    case AST_NODE_TYPE_BINARY_OPERATION:
        result = eval_constant_binary_operation(node);
        break;

    case AST_NODE_TYPE_CONDITIONAL_OPERATION:
        result = eval_constant_conditional_operation(node);
        break;

    default:
        result.is_valid = false;
        break;
    }

    return result;
}
