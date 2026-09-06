#include "parser.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

AST_node_t *expression(void);
AST_node_t *assignment_expression(void);
AST_node_t *conditional_expression(void);
AST_node_t *logical_or_expression(void);
AST_node_t *logical_and_expression(void);
AST_node_t *bitwise_or_expression(void);
AST_node_t *bitwise_xor_expression(void);
AST_node_t *bitwise_and_expression(void);
AST_node_t *equality_expression(void);
AST_node_t *relational_expression(void);
AST_node_t *shift_expression(void);
AST_node_t *additive_expression(void);
AST_node_t *multiplicative_expression(void);
AST_node_t *unary_expression(void);
AST_node_t *postfix_expression(void);
AST_node_t *argument_expression_list_opt(void);
AST_node_t *primary_expression(void);
AST_node_t *translation_unit(void);

static AST_node_t *create_AST_node(AST_node_type_t type)
{
    AST_node_t *node = malloc(sizeof(AST_node_t));
    if (!node)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    node->type = type;
    array_create(token_t *, node->tokens);
    array_create(AST_node_t *, node->children);

    return node;
}

static void node_push_child(AST_node_t *node, AST_node_t *child)
{
    ASSERT(node);
    array_push(node->children, child);
}

static void node_push_token(AST_node_t *node, token_t *tok)
{
    ASSERT(node);
    array_push(node->tokens, tok);
}

static void node_start(AST_node_t *node, position_t *start)
{
    ASSERT(node);
    node->start = *start;
}

static void node_end(AST_node_t *node, position_t *end)
{
    ASSERT(node);
    node->end = *end;
}

AST_node_t *parse(void)
{
    return translation_unit();
}

AST_node_t *translation_unit(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_TRANSLATION_UNIT);
    while (tok_peek()->type != TOKTYPE_EOF)
        // node_push_child(node, external_declaration());
        node_push_child(node, expression());

    if (has_errors())
        return NULL;

    position_t pos = {.col = 0, .row = 0};
    node_start(node, &pos);
    node_end(node, &pos);

    if (array_size(node->children) >= 1)
    {
        node_start(node, &(node->children[0]->start));
        node_end(node, &(node->children[array_size(node->children) - 1])->end);
    }

    return node;
}

static bool is_unary_operator(token_type_t type)
{
    switch (type)
    {
    case TOKTYPE_PLUS_PLUS:
    case TOKTYPE_MINUS_MINUS:
    case TOKTYPE_AMPERSAND:
    case TOKTYPE_STAR:
    case TOKTYPE_PLUS:
    case TOKTYPE_MINUS:
    case TOKTYPE_TILDE:
    case TOKTYPE_BANG:
        return true;

    default:
        return false;
    }
}

AST_node_t *unary_expression(void)
{
    token_t *tok = tok_peek();

    if (!is_unary_operator(tok->type))
        return postfix_expression();

    AST_node_t *node = create_AST_node(AST_NODE_TYPE_UNARY_OPERATION);

    node_push_token(node, tok_next());
    node_push_child(node, unary_expression());

    return node;
}

static bool is_postfix_operator(token_type_t type)
{
    switch (type)
    {
    case TOKTYPE_LBRACKET:
    case TOKTYPE_LPAREN:
    case TOKTYPE_DOT:
    case TOKTYPE_ARROW:
    case TOKTYPE_PLUS_PLUS:
    case TOKTYPE_MINUS_MINUS:
        return true;

    default:
        return false;
    }
}

AST_node_t *postfix_expression(void)
{
    AST_node_t *node = primary_expression();

    while (is_postfix_operator(tok_peek()->type))
    {
        token_t *tok = tok_next();

        AST_node_t *postfix = create_AST_node(AST_NODE_TYPE_POSTFIX_OPERATION);
        node_push_child(postfix, node);

        switch (tok->type)
        {
        case TOKTYPE_LBRACKET:
            postfix->type = AST_NODE_TYPE_ARRAY_SUBSCRIPT;
            node_push_child(postfix, expression());
            tok_expect(TOKTYPE_RBRACKET);
            break;

        case TOKTYPE_LPAREN:
            postfix->type = AST_NODE_TYPE_FUNCTION_CALL;
            node_push_child(postfix, argument_expression_list_opt());
            tok_expect(TOKTYPE_RPAREN);
            break;

        case TOKTYPE_DOT:
            postfix->type = AST_NODE_TYPE_MEMBER_ACCESS;
            node_push_token(postfix, tok_expect(TOKTYPE_IDENTIFIER));
            break;

        case TOKTYPE_ARROW:
            postfix->type = AST_NODE_TYPE_POINTER_MEMBER_ACCESS;
            node_push_token(postfix, tok_expect(TOKTYPE_IDENTIFIER));
            break;

        case TOKTYPE_PLUS_PLUS:
        case TOKTYPE_MINUS_MINUS:
            postfix->type = AST_NODE_TYPE_POSTFIX_OPERATION;
            node_push_token(postfix, tok);
            break;

        default:
            break;
        }

        node = postfix;
    }

    return node;
}

AST_node_t *argument_expression_list_opt(void)
{
    if (tok_peek()->type == TOKTYPE_RPAREN)
    {
        return NULL;
    }

    AST_node_t *node = assignment_expression();

    while (tok_peek()->type == TOKTYPE_COMMA)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);
        node_push_token(expr, tok_next());
        node_push_child(expr, node);
        node_push_child(expr, assignment_expression());
        node = expr;
    }

    return node;
}

AST_node_t *primary_expression(void)
{
    token_t *tok = tok_peek();
    switch (tok->type)
    {
    case TOKTYPE_IDENTIFIER:
    {
        AST_node_t *node = create_AST_node(AST_NODE_TYPE_IDENTIFIER);
        node_push_token(node, tok_next());
        return node;
    }
    case TOKTYPE_INTLIT:
    {
        AST_node_t *node = create_AST_node(AST_NODE_TYPE_INTEGER_LITERAL);
        node_push_token(node, tok_next());
        return node;
    }
    case TOKTYPE_LPAREN:
    {
        tok_next();

        AST_node_t *node = expression();
        tok_expect(TOKTYPE_RPAREN);
        return node;
    }
    default:
    {
        push_error(&tok->start, &tok->end, "expected identifier, integer literal, '('");
        tok_next();
        return NULL;
    }
    }
}

AST_node_t *expression(void)
{
    AST_node_t *node = assignment_expression();

    while (tok_peek()->type == TOKTYPE_COMMA)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, assignment_expression());

        node = expr;
    }

    return node;
}

static bool is_assignment_operator(token_type_t type)
{
    switch (type)
    {
    case TOKTYPE_EQUAL:
    case TOKTYPE_MUL_ASSIGN:
    case TOKTYPE_DIV_ASSIGN:
    case TOKTYPE_MOD_ASSIGN:
    case TOKTYPE_ADD_ASSIGN:
    case TOKTYPE_SUB_ASSIGN:
    case TOKTYPE_LEFT_ASSIGN:
    case TOKTYPE_RIGHT_ASSIGN:
    case TOKTYPE_AND_ASSIGN:
    case TOKTYPE_XOR_ASSIGN:
    case TOKTYPE_OR_ASSIGN:
        return true;

    default:
        return false;
    }
}

AST_node_t *assignment_expression(void)
{
    size_t checkpoint = tokstream_checkpoint();

    AST_node_t *left = unary_expression();

    if (is_assignment_operator(tok_peek()->type))
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_ASSIGNMENT);

        node_push_child(expr, left);
        node_push_token(expr, tok_next());
        node_push_child(expr, assignment_expression());

        return expr;
    }

    tokstream_restore(checkpoint);

    return conditional_expression();
}

AST_node_t *conditional_expression(void)
{
    AST_node_t *condition = logical_or_expression();

    if (tok_peek()->type != TOKTYPE_QUESTION)
        return condition;

    tok_next();

    AST_node_t *node = create_AST_node(AST_NODE_TYPE_CONDITIONAL_OPERATION);

    node_push_child(node, condition);
    node_push_child(node, expression());

    tok_expect(TOKTYPE_COLON);

    node_push_child(node, conditional_expression());

    return node;
}

static AST_node_t *left_associative_expression(token_type_t op, AST_node_type_t node_type, AST_node_t *(*func)(void))
{
    AST_node_t *node = func();

    while (tok_peek()->type == op)
    {
        AST_node_t *expr = create_AST_node(node_type);
        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, func());
        node = expr;
    }

    return node;
}

AST_node_t *logical_or_expression(void)
{
    return left_associative_expression(TOKTYPE_OR_OR, AST_NODE_TYPE_BINARY_OPERATION, logical_and_expression);
}

AST_node_t *logical_and_expression(void)
{
    return left_associative_expression(TOKTYPE_AND_AND, AST_NODE_TYPE_BINARY_OPERATION, bitwise_or_expression);
}

AST_node_t *bitwise_or_expression(void)
{
    return left_associative_expression(TOKTYPE_PIPE, AST_NODE_TYPE_BINARY_OPERATION, bitwise_xor_expression);
}

AST_node_t *bitwise_xor_expression(void)
{
    return left_associative_expression(TOKTYPE_CARET, AST_NODE_TYPE_BINARY_OPERATION, bitwise_and_expression);
}

AST_node_t *bitwise_and_expression(void)
{
    return left_associative_expression(TOKTYPE_AMPERSAND, AST_NODE_TYPE_BINARY_OPERATION, equality_expression);
}

AST_node_t *equality_expression(void)
{
    AST_node_t *node = relational_expression();

    while (tok_peek()->type == TOKTYPE_EQUAL_EQUAL || tok_peek()->type == TOKTYPE_NOT_EQUAL)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);
        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, relational_expression());
        node = expr;
    }

    return node;
}

AST_node_t *relational_expression(void)
{
    AST_node_t *node = shift_expression();

    while (tok_peek()->type == TOKTYPE_LESS || tok_peek()->type == TOKTYPE_GREATER || tok_peek()->type == TOKTYPE_LESS_EQUAL || tok_peek()->type == TOKTYPE_GREATER_EQUAL)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);
        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, shift_expression());
        node = expr;
    }

    return node;
}

AST_node_t *shift_expression(void)
{
    AST_node_t *node = additive_expression();

    while (tok_peek()->type == TOKTYPE_SHIFT_LEFT || tok_peek()->type == TOKTYPE_SHIFT_RIGHT)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);
        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, additive_expression());
        node = expr;
    }

    return node;
}

AST_node_t *additive_expression(void)
{
    AST_node_t *node = multiplicative_expression();

    while (tok_peek()->type == TOKTYPE_PLUS || tok_peek()->type == TOKTYPE_MINUS)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);
        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, multiplicative_expression());
        node = expr;
    }

    return node;
}

AST_node_t *multiplicative_expression(void)
{
    AST_node_t *node = unary_expression();

    while (tok_peek()->type == TOKTYPE_STAR || tok_peek()->type == TOKTYPE_SLASH || tok_peek()->type == TOKTYPE_PERCENT)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);
        node_push_child(expr, node);
        node_push_token(expr, tok_next());
        node_push_child(expr, unary_expression());
        node = expr;
    }

    return node;
}
