#include "parser.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

AST_node_t *parse(void);
AST_node_t *translation_unit(void);
AST_node_t *external_declaration(void);
AST_node_t *function_definition(void);
AST_node_t *declaration(void);
AST_node_t *init_declarator(void);
AST_node_t *initializer(void);
AST_node_t *type_specifier(void);
AST_node_t *declarator(void);
AST_node_t *direct_declarator(void);
AST_node_t *pointer(void);
AST_node_t *parameter_declaration(void);
AST_node_t *constant_expression(void);
AST_node_t *statement(void);
AST_node_t *compound_statement(void);
AST_node_t *block_item(void);
AST_node_t *expression_statement(void);
AST_node_t *selection_statement(void);
AST_node_t *iteration_statement(void);
AST_node_t *jump_statement(void);
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

static void parameter_list(AST_node_t *node);

void free_AST_node(AST_node_t *node)
{
    AST_node_t *it;
    foreach (node->children, it)
    {
        free_AST_node(it);
    }

    array_free(node->children);
    array_free(node->tokens);
    free(node);
}

void free_AST_node_layer(AST_node_t *node)
{
    array_free(node->children);
    array_free(node->tokens);
    free(node);
}

AST_node_t *create_AST_node(AST_node_type_t type)
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

    if (tok_peek())
    {
        node->start = tok_peek()->start;
        node->end = tok_peek()->end;
    }
    else
    {
        position_t pos = {.col = 0, .row = 0};
        node->start = pos;
        node->end = pos;
    }

    return node;
}

void node_push_child(AST_node_t *node, AST_node_t *child)
{
    ASSERT(node);

    if (!child)
        return;

    array_push(node->children, child);
}

void node_push_token(AST_node_t *node, token_t *tok)
{
    ASSERT(node);
    ASSERT(tok);

    array_push(node->tokens, tok);
}

AST_node_t *node_clone(AST_node_t *node)
{
    AST_node_t *res = create_AST_node(node->type);
    res->start = node->start;
    res->end = node->end;
    res->symbol = node->symbol;

    token_t *tok;
    foreach (node->tokens, tok)
    {
        node_push_token(res, tok);
    }

    AST_node_t *child;
    foreach (node->children, child)
    {
        node_push_child(res, node_clone(child));
    }

    return res;
}

static void node_start(AST_node_t *node, position_t *start)
{
    ASSERT(node);
    ASSERT(start);

    node->start = *start;
}

static void node_end(AST_node_t *node, position_t *end)
{
    ASSERT(node);
    ASSERT(end);

    node->end = *end;
}

static void node_start_token(AST_node_t *node, token_t *tok)
{
    ASSERT(node);
    ASSERT(tok);

    node->start = tok->start;
}

static void node_end_token(AST_node_t *node, token_t *tok)
{
    ASSERT(node);
    ASSERT(tok);

    node->end = tok->end;
}

static void node_span(AST_node_t *node, AST_node_t *first, AST_node_t *last)
{
    ASSERT(node);

    if (!first || !last)
        return;

    ASSERT(first);
    ASSERT(last);

    node->start = first->start;
    node->end = last->end;
}

static void node_span_from_children(AST_node_t *node)
{
    ASSERT(node);

    size_t count = array_size(node->children);

    if (count == 0)
        return;

    AST_node_t *first = node->children[0];
    AST_node_t *last = node->children[count - 1];

    ASSERT(first);
    ASSERT(last);

    node->start = first->start;
    node->end = last->end;
}

AST_node_t *parse(void)
{
    return translation_unit();
}

AST_node_t *translation_unit(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_TRANSLATION_UNIT);

    while (tok_peek()->type != TOKTYPE_EOF)
        node_push_child(node, external_declaration());

    if (has_errors())
        return NULL;

    if (array_size(node->children) >= 1)
    {
        node_start(node, &(node->children[0]->start));
        node_end(node, &(node->children[array_size(node->children) - 1]->end));
    }
    else
    {
        position_t pos = tok_peek()->start;
        node_start(node, &pos);
        node_end(node, &pos);
    }

    return node;
}

AST_node_t *external_declaration(void)
{
    size_t checkpoint = tokstream_checkpoint();
    type_specifier();
    declarator();

    token_t *tok = tok_peek();
    if (tok->type == TOKTYPE_EQUAL || tok->type == TOKTYPE_SEMICOLON)
    {
        tokstream_restore(checkpoint);
        return declaration();
    }

    tokstream_restore(checkpoint);
    return function_definition();
}

AST_node_t *function_definition(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_FUNCTION_DEFINITION);

    AST_node_t *type = type_specifier();
    node_push_child(node, type);

    AST_node_t *decl = declarator();
    node_push_child(node, decl);

    AST_node_t *body = compound_statement();
    node_push_child(node, body);

    node_span(node, type, body);

    return node;
}

static bool is_declaration_start(token_type_t type)
{
    switch (type)
    {
    case TOKTYPE_KW_INT:
    case TOKTYPE_KW_VOID:
        return true;

    default:
        return false;
    }
}

AST_node_t *type_specifier(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_BUILTIN_TYPE);

    token_t *tok = tok_expect_n(2, TOKTYPE_KW_INT, TOKTYPE_KW_VOID);

    node_push_token(node, tok);

    node_start_token(node, tok);
    node_end_token(node, tok);

    return node;
}

AST_node_t *declaration(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_DECLARATION);

    AST_node_t *type = type_specifier();
    node_push_child(node, type);

    AST_node_t *init = init_declarator();
    node_push_child(node, init);

    while (tok_peek()->type == TOKTYPE_COMMA)
    {
        tok_next();

        AST_node_t *child = init_declarator();
        node_push_child(node, child);
    }

    token_t *semicolon = tok_expect(TOKTYPE_SEMICOLON);

    node_span(node, type, node->children[array_size(node->children) - 1]);

    node_end_token(node, semicolon);

    return node;
}

AST_node_t *init_declarator(void)
{
    AST_node_t *node = declarator();

    if (tok_peek()->type == TOKTYPE_EQUAL)
    {
        token_t *equal = tok_next();

        AST_node_t *binop = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        node_push_child(binop, node);
        node_push_token(binop, equal);

        AST_node_t *init = initializer();
        node_push_child(binop, init);

        node_span(binop, node, init);

        node = binop;
    }

    return node;
}

AST_node_t *initializer(void)
{
    return assignment_expression();
}

AST_node_t *direct_declarator(void)
{
    AST_node_t *node;

    if (tok_peek()->type == TOKTYPE_IDENTIFIER)
    {
        node = create_AST_node(AST_NODE_TYPE_DECLARATOR);

        token_t *tok = tok_next();
        node_push_token(node, tok);

        node_start_token(node, tok);
        node_end_token(node, tok);
    }
    else if (tok_peek()->type == TOKTYPE_LPAREN)
    {
        token_t *left_paren = tok_next();

        node = declarator();

        token_t *right_paren = tok_expect(TOKTYPE_RPAREN);

        node_start_token(node, left_paren);
        node_end_token(node, right_paren);
    }
    else
    {
        token_t *tok = tok_next();

        push_error(&tok->start, &tok->end, "expected identifier or '('");
        return NULL;
    }

    while (true)
    {
        if (tok_peek()->type == TOKTYPE_LBRACKET)
        {
            token_t *left_bracket = tok_next();

            AST_node_t *array = create_AST_node(AST_NODE_TYPE_ARRAY_TYPE);

            node_push_child(array, node);

            if (tok_peek()->type != TOKTYPE_RBRACKET)
                node_push_child(array, constant_expression());

            token_t *right_bracket = tok_expect(TOKTYPE_RBRACKET);

            node_start_token(array, left_bracket);
            node_end_token(array, right_bracket);

            node_start(array, &node->start);

            node = array;
        }
        else if (tok_peek()->type == TOKTYPE_LPAREN)
        {
            token_t *left_paren = tok_next();

            AST_node_t *function = create_AST_node(AST_NODE_TYPE_FUNCTION_TYPE);

            node_push_child(function, node);

            if (tok_peek()->type != TOKTYPE_RPAREN)
                parameter_list(function);

            token_t *right_paren = tok_expect(TOKTYPE_RPAREN);

            node_start_token(function, left_paren);
            node_end_token(function, right_paren);

            node_start(function, &node->start);

            node = function;
        }
        else
        {
            break;
        }
    }

    return node;
}

AST_node_t *declarator(void)
{
    AST_node_t *node = NULL;
    AST_node_t *p = NULL;

    if (tok_peek()->type == TOKTYPE_STAR)
    {
        AST_node_t *pointer_node = pointer();
        p = pointer_node;

        while (array_size(p->children) != 0)
            p = p->children[0];

        node = pointer_node;
    }

    if (node)
    {
        ASSERT(p);

        AST_node_t *child = direct_declarator();
        node_push_child(p, child);

        node_span(node, node, child);

        return node;
    }

    return direct_declarator();
}

AST_node_t *pointer(void)
{
    token_t *star = tok_expect(TOKTYPE_STAR);

    AST_node_t *node = create_AST_node(AST_NODE_TYPE_POINTER_TYPE);

    node_push_token(node, star);

    node_start_token(node, star);
    node_end_token(node, star);

    if (tok_peek()->type == TOKTYPE_STAR)
    {
        AST_node_t *child = pointer();
        node_push_child(node, child);

        node_end(node, &child->end);
    }

    return node;
}

static void parameter_list(AST_node_t *node)
{
    ASSERT(node);

    node_push_child(node, parameter_declaration());

    while (tok_peek()->type == TOKTYPE_COMMA)
    {
        tok_next();
        node_push_child(node, parameter_declaration());
    }
}

AST_node_t *parameter_declaration(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_PARAMETER);

    AST_node_t *type = type_specifier();
    node_push_child(node, type);

    AST_node_t *decl = declarator();
    node_push_child(node, decl);

    node_span(node, type, decl);

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

    token_t *operator = tok_next();

    node_push_token(node, operator);
    node_start_token(node, operator);

    AST_node_t *child = unary_expression();
    node_push_child(node, child);

    node_end(node, &child->end);

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
        node_start(postfix, &node->start);

        switch (tok->type)
        {
        case TOKTYPE_LBRACKET:
        {
            postfix->type = AST_NODE_TYPE_ARRAY_SUBSCRIPT;

            AST_node_t *index = expression();
            node_push_child(postfix, index);

            token_t *right_bracket = tok_expect(TOKTYPE_RBRACKET);

            node_end_token(postfix, right_bracket);
            break;
        }

        case TOKTYPE_LPAREN:
        {
            postfix->type = AST_NODE_TYPE_FUNCTION_CALL;

            AST_node_t *args = argument_expression_list_opt();

            if (args)
                node_push_child(postfix, args);

            token_t *right_paren = tok_expect(TOKTYPE_RPAREN);

            node_end_token(postfix, right_paren);
            break;
        }

        case TOKTYPE_DOT:
            postfix->type = AST_NODE_TYPE_MEMBER_ACCESS;

            node_push_token(
                postfix,
                tok_expect(TOKTYPE_IDENTIFIER));

            node_end(postfix,
                     &postfix->tokens[array_size(postfix->tokens) - 1]->end);
            break;

        case TOKTYPE_ARROW:
            postfix->type = AST_NODE_TYPE_POINTER_MEMBER_ACCESS;

            node_push_token(
                postfix,
                tok_expect(TOKTYPE_IDENTIFIER));

            node_end(postfix,
                     &postfix->tokens[array_size(postfix->tokens) - 1]->end);
            break;

        case TOKTYPE_PLUS_PLUS:
        case TOKTYPE_MINUS_MINUS:
            postfix->type = AST_NODE_TYPE_POSTFIX_OPERATION;

            node_push_token(postfix, tok);
            node_end_token(postfix, tok);
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
        return NULL;

    AST_node_t *node = assignment_expression();

    while (tok_peek()->type == TOKTYPE_COMMA)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        node_push_child(expr, node);

        token_t *comma = tok_next();
        node_push_token(expr, comma);

        AST_node_t *right = assignment_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

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

        token_t *identifier = tok_next();

        node_push_token(node, identifier);

        node_start_token(node, identifier);
        node_end_token(node, identifier);

        return node;
    }

    case TOKTYPE_INTLIT:
    {
        AST_node_t *node = create_AST_node(AST_NODE_TYPE_INTEGER_LITERAL);

        token_t *literal = tok_next();

        node_push_token(node, literal);

        node_start_token(node, literal);
        node_end_token(node, literal);

        return node;
    }

    case TOKTYPE_LPAREN:
    {
        token_t *left_paren = tok_next();

        AST_node_t *node = expression();

        token_t *right_paren = tok_expect(TOKTYPE_RPAREN);

        node_start_token(node, left_paren);
        node_end_token(node, right_paren);

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

AST_node_t *constant_expression(void)
{
    return conditional_expression();
}

AST_node_t *expression(void)
{
    AST_node_t *node = assignment_expression();

    while (tok_peek()->type == TOKTYPE_COMMA)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        node_push_child(expr, node);

        token_t *comma = tok_next();
        node_push_token(expr, comma);

        AST_node_t *right = assignment_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

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

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = assignment_expression();
        node_push_child(expr, right);

        node_span(expr, left, right);

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

    token_t *question = tok_next();

    AST_node_t *node = create_AST_node(AST_NODE_TYPE_CONDITIONAL_OPERATION);

    node_push_child(node, condition);
    node_start(node, &condition->start);

    AST_node_t *child1 = expression();
    node_push_child(node, child1);

    tok_expect(TOKTYPE_COLON);

    AST_node_t *child2 = conditional_expression();
    node_push_child(node, child2);

    node_end(node, &child2->end);

    node_push_token(node, question);

    return node;
}

static AST_node_t *left_associative_expression(
    token_type_t op,
    AST_node_type_t node_type,
    AST_node_t *(*func)(void))
{
    AST_node_t *node = func();

    while (tok_peek()->type == op)
    {
        AST_node_t *expr = create_AST_node(node_type);

        node_push_child(expr, node);

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = func();
        node_push_child(expr, right);

        node_span(expr, node, right);

        node = expr;
    }

    return node;
}

AST_node_t *logical_or_expression(void)
{
    return left_associative_expression(
        TOKTYPE_OR_OR,
        AST_NODE_TYPE_BINARY_OPERATION,
        logical_and_expression);
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

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = relational_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

        node = expr;
    }

    return node;
}

AST_node_t *relational_expression(void)
{
    AST_node_t *node = shift_expression();

    while (tok_peek()->type == TOKTYPE_LESS ||
           tok_peek()->type == TOKTYPE_GREATER ||
           tok_peek()->type == TOKTYPE_LESS_EQUAL ||
           tok_peek()->type == TOKTYPE_GREATER_EQUAL)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        node_push_child(expr, node);

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = shift_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

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

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = additive_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

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

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = multiplicative_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

        node = expr;
    }

    return node;
}

AST_node_t *multiplicative_expression(void)
{
    AST_node_t *node = unary_expression();

    while (tok_peek()->type == TOKTYPE_STAR ||
           tok_peek()->type == TOKTYPE_SLASH ||
           tok_peek()->type == TOKTYPE_PERCENT)
    {
        AST_node_t *expr = create_AST_node(AST_NODE_TYPE_BINARY_OPERATION);

        node_push_child(expr, node);

        token_t *operator = tok_next();
        node_push_token(expr, operator);

        AST_node_t *right = unary_expression();
        node_push_child(expr, right);

        node_span(expr, node, right);

        node = expr;
    }

    return node;
}

AST_node_t *statement(void)
{
    token_t *tok = tok_peek();

    if (tok->type == TOKTYPE_LBRACE)
        return compound_statement();

    else if (tok->type == TOKTYPE_KW_IF)
        return selection_statement();

    else if (tok->type == TOKTYPE_KW_WHILE ||
             tok->type == TOKTYPE_KW_FOR)
        return iteration_statement();

    else if (tok->type == TOKTYPE_KW_RETURN)
        return jump_statement();

    return expression_statement();
}

AST_node_t *compound_statement(void)
{
    token_t *left_brace = tok_expect(TOKTYPE_LBRACE);

    AST_node_t *node = create_AST_node(AST_NODE_TYPE_COMPOUND_STATEMENT);

    node_start_token(node, left_brace);

    while (tok_peek()->type != TOKTYPE_RBRACE)
    {
        if (tok_peek()->type == TOKTYPE_EOF)
        {
            push_error(&tok_peek()->start,
                       &tok_peek()->end,
                       "unexpected EOF");

            node_end(node, &tok_peek()->start);

            return node;
        }

        node_push_child(node, block_item());
    }

    token_t *right_brace = tok_next();

    node_end_token(node, right_brace);

    return node;
}

AST_node_t *block_item(void)
{
    if (is_declaration_start(tok_peek()->type))
        return declaration();

    return statement();
}

AST_node_t *expression_statement(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_EXPRESSION_STATEMENT);

    if (tok_peek()->type == TOKTYPE_SEMICOLON)
    {
        token_t *semicolon = tok_next();

        node_start_token(node, semicolon);
        node_end_token(node, semicolon);

        return node;
    }

    AST_node_t *expr = expression();
    if (!expr)
    {
        tok_expect(TOKTYPE_SEMICOLON);
        return node;
    }
    node_push_child(node, expr);

    token_t *semicolon = tok_expect(TOKTYPE_SEMICOLON);

    node_start(node, &expr->start);
    node_end_token(node, semicolon);

    return node;
}

AST_node_t *selection_statement(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_IF_STATEMENT);

    token_t *if_token = tok_expect(TOKTYPE_KW_IF);

    node_start_token(node, if_token);

    tok_expect(TOKTYPE_LPAREN);

    AST_node_t *condition = expression();
    node_push_child(node, condition);

    tok_expect(TOKTYPE_RPAREN);

    AST_node_t *then_statement = statement();
    node_push_child(node, then_statement);

    node_end(node, &then_statement->end);

    if (tok_peek()->type == TOKTYPE_KW_ELSE)
    {
        tok_next();

        AST_node_t *else_statement = statement();
        node_push_child(node, else_statement);

        node_end(node, &else_statement->end);
    }

    return node;
}

AST_node_t *iteration_statement(void)
{
    token_t *tok = tok_peek();

    if (tok->type == TOKTYPE_KW_WHILE)
    {
        AST_node_t *node = create_AST_node(AST_NODE_TYPE_WHILE_STATEMENT);

        token_t *while_token = tok_next();

        node_start_token(node, while_token);

        tok_expect(TOKTYPE_LPAREN);

        AST_node_t *condition = expression();
        node_push_child(node, condition);

        tok_expect(TOKTYPE_RPAREN);

        AST_node_t *body = statement();
        node_push_child(node, body);

        node_end(node, &body->end);

        return node;
    }

    AST_node_t *node = create_AST_node(AST_NODE_TYPE_FOR_STATEMENT);

    token_t *for_token = tok_expect(TOKTYPE_KW_FOR);

    node_start_token(node, for_token);

    tok_expect(TOKTYPE_LPAREN);

    if (is_declaration_start(tok_peek()->type))
    {
        node_push_child(node, declaration());
    }
    else
    {
        node_push_child(node, expression_statement());
    }

    node_push_child(node, expression_statement());

    if (tok_peek()->type != TOKTYPE_RPAREN)
        node_push_child(node, expression());

    tok_expect(TOKTYPE_RPAREN);

    AST_node_t *body = statement();
    node_push_child(node, body);

    node_end(node, &body->end);

    return node;
}

AST_node_t *jump_statement(void)
{
    AST_node_t *node = create_AST_node(AST_NODE_TYPE_RETURN_STATEMENT);

    token_t *return_token = tok_expect(TOKTYPE_KW_RETURN);

    node_start_token(node, return_token);

    if (tok_peek()->type == TOKTYPE_SEMICOLON)
    {
        token_t *semicolon = tok_next();

        node_end_token(node, semicolon);

        return node;
    }

    AST_node_t *expr = expression();
    node_push_child(node, expr);

    token_t *semicolon = tok_expect(TOKTYPE_SEMICOLON);

    node_end_token(node, semicolon);

    return node;
}
