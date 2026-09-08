#include "sema.h"
#include "strdup.h"

static size_t max(size_t a, size_t b)
{
    if (a > b)
        return a;
    return b;
}

static type_t *create_type(void)
{
    type_t *res = malloc(sizeof(type_t));
    if (!res)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    return res;
}

static type_t *clone_type(type_t *t)
{
    type_t *res = create_type();
    res->kind = t->kind;

    if (res->kind == TYPE_POINTER)
    {
        res->pointer.dest = clone_type(t->pointer.dest);
    }

    else if (res->kind == TYPE_ARRAY)
    {
        res->array.size = t->array.size;
        res->array.element = clone_type(t->array.element);
    }

    else if (res->kind == TYPE_FUNCTION)
    {
        static_array_create(type_t *, static_array_size(t->function.parameter_types), res->function.parameter_types);

        for (size_t i = 0; i < static_array_size(t->function.parameter_types); i++)
            static_array_push(res->function.parameter_types, clone_type(t->function.parameter_types[i]));

        res->function.return_type = clone_type(t->function.return_type);
    }

    return res;
}

static bool compare_types(type_t *left, type_t *right)
{
    if ((left->kind == TYPE_INTLIT && right->kind == TYPE_INT) || (left->kind == TYPE_INT && right->kind == TYPE_INTLIT))
        return true;

    if (left->kind != right->kind)
        return false;

    if (left->kind == TYPE_POINTER)
    {
        return compare_types(left->pointer.dest, right->pointer.dest);
    }

    else if (left->kind == TYPE_ARRAY)
    {
        return compare_types(left->array.element, right->array.element);
    }

    else if (left->kind == TYPE_FUNCTION)
    {
        if (static_array_size(left->function.parameter_types) != static_array_size(right->function.parameter_types))
            return false;

        for (size_t i = 0; i < static_array_size(left->function.parameter_types); i++)
            if (!compare_types(left->function.parameter_types[i], right->function.parameter_types[i]))
                return false;

        return compare_types(left->function.return_type, right->function.return_type);
    }

    return true;
}

static type_t *process_builtin_type(AST_node_t *node)
{
    ASSERT(node);

    type_t *res = create_type();

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_KW_VOID:
        res->kind = TYPE_VOID;
        break;

    case TOKTYPE_KW_INT:
        res->kind = TYPE_INT;
        break;

    default:
        ASSERT(0);
    }

    return res;
}

static type_t *process_simple_type(AST_node_t *node)
{
    if (!node)
        ASSERT(node);

    switch (node->type)
    {
    case AST_NODE_TYPE_BUILTIN_TYPE:
        return process_builtin_type(node);

    default:
        ASSERT(0);
    }
}

static type_t *process_type(AST_node_t *node, AST_node_t **out_node)
{
    ASSERT(node);

    type_t *type = process_simple_type(node->children[0]);
    AST_node_t *current_node = node->children[1];

    while (current_node->type == AST_NODE_TYPE_POINTER_TYPE || current_node->type == AST_NODE_TYPE_ARRAY_TYPE || current_node->type == AST_NODE_TYPE_BINARY_OPERATION)
    {
        if (current_node->type == AST_NODE_TYPE_POINTER_TYPE)
        {
            type_t *t = create_type();
            t->kind = TYPE_POINTER;
            t->pointer.dest = type;

            type = t;
        }
        else if (current_node->type == AST_NODE_TYPE_ARRAY_TYPE)
        {
            type_t *t = create_type();
            t->kind = TYPE_ARRAY;
            t->array.element = type;

            symbol_const_value_t size_val = eval_constant_expression(current_node->children[1]);
            if (!size_val.is_valid || (size_val.type->kind != TYPE_INT && size_val.type->kind != TYPE_INTLIT))
            {
                // TODO: VAR?

                if (size_val.is_valid)
                    free_type(size_val.type);

                free_type(type);

                push_error(&current_node->start, &current_node->end, "invalid size for array");
                return NULL;
            }
            free_type(size_val.type);
            t->array.size = size_val.val;

            type = t;
        }

        current_node = current_node->children[0];
    }

    if (out_node)
        *out_node = current_node;

    return type;
}

static void process_declaration(AST_node_t *declaration)
{
    ASSERT(declaration);

    symbol_t sym;
    sym.kind = SYMBOL_VARIABLE;
    sym.declaration = declaration;

    AST_node_t *func_type_or_declarator;
    sym.type = process_type(declaration, &func_type_or_declarator);

    symbol_const_value_t sym_val; // TODO: default values
    sym_val.is_valid = false;
    if (declaration->children[1]->type == AST_NODE_TYPE_BINARY_OPERATION)
    {
        sym_val = eval_constant_expression(declaration->children[1]->children[1]);
    }

    if (!sym.type)
    {
        if (sym_val.is_valid)
            free_type(sym_val.type);

        return;
    }

    if (func_type_or_declarator->type == AST_NODE_TYPE_FUNCTION_TYPE)
    {
        ASSERT(array_size(func_type_or_declarator->children) >= 1);
        sym.kind = SYMBOL_FUNCTION;

        type_t *t = create_type();
        t->kind = TYPE_FUNCTION;
        t->function.return_type = sym.type;
        static_array_create(type_t *, max(array_size(func_type_or_declarator->children) - 1, 1), t->function.parameter_types);

        for (size_t i = 1; i < array_size(func_type_or_declarator->children); i++)
        {
            array_push(t->function.parameter_types, process_type(func_type_or_declarator->children[i], NULL));
        }

        sym.type = t;
        func_type_or_declarator = func_type_or_declarator->children[0];
    }

    if (sym_val.is_valid && !compare_types(sym_val.type, sym.type))
    {
        if (sym_val.is_valid)
            free_type(sym_val.type);

        free_type(sym.type);
        push_error(&declaration->start, &declaration->end, "invalid type conversion");

        return;
    }

    sym.value = sym_val;

    ASSERT(func_type_or_declarator->type == AST_NODE_TYPE_DECLARATOR);
    ASSERT(func_type_or_declarator->tokens[0]->type == TOKTYPE_IDENTIFIER);
    sym.name = strdup(func_type_or_declarator->tokens[0]->text);

    symbol_insert(sym);
}

static void process_parameter(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_PARAMETER);

    symbol_t sym;
    sym.kind = SYMBOL_PARAMETER;
    sym.declaration = node;
    sym.value.is_valid = false;

    AST_node_t *declarator;
    sym.type = process_type(node, &declarator);

    ASSERT(declarator->type == AST_NODE_TYPE_DECLARATOR);
    ASSERT(declarator->tokens[0]->type == TOKTYPE_IDENTIFIER);
    sym.name = strdup(declarator->tokens[0]->text);

    symbol_insert(sym);
}

static void process_AST_node(AST_node_t *node);
static void process_function_definition(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(array_size(node->children) >= 1);

    symbol_t sym;
    sym.kind = SYMBOL_FUNCTION;
    sym.declaration = node;
    sym.value.is_valid = false;

    AST_node_t *func_type = node->children[1];
    ASSERT(array_size(func_type->children) >= 1);

    type_t *t = create_type();
    t->kind = TYPE_FUNCTION;
    t->function.return_type = process_simple_type(node->children[0]);
    static_array_create(type_t *, max(array_size(func_type->children) - 1, 1), t->function.parameter_types);

    begin_scope();
    for (size_t i = 1; i < array_size(func_type->children); i++)
    {
        array_push(t->function.parameter_types, process_type(func_type->children[i], NULL));
        process_parameter(func_type->children[i]);
    }
    process_AST_node(node->children[2]);

    end_scope();

    sym.name = strdup(func_type->children[0]->tokens[0]->text);
    sym.type = t;

    symbol_insert(sym);
}

static void process_compound_statement(AST_node_t *node)
{
    ASSERT(node);
    for (size_t i = 0; i < array_size(node->children); i++)
    {
        if (node->children[i] == NULL)
            continue;

        AST_node_t *child = node->children[i];
        if (child->type == AST_NODE_TYPE_COMPOUND_STATEMENT)
        {
            begin_scope();
            process_AST_node(child);
            end_scope();
        }
        else
            process_AST_node(child);
    }
}

static void process_AST_node(AST_node_t *node)
{
    switch (node->type)
    {
    case AST_NODE_TYPE_DECLARATION:
        process_declaration(node);
        break;

    case AST_NODE_TYPE_FUNCTION_DEFINITION:
        process_function_definition(node);
        break;

    case AST_NODE_TYPE_COMPOUND_STATEMENT:
        process_compound_statement(node);
        break;

    case AST_NODE_TYPE_EXPRESSION_STATEMENT:
        // TODO: check types
        break;

    case AST_NODE_TYPE_IF_STATEMENT:
    case AST_NODE_TYPE_WHILE_STATEMENT:
    case AST_NODE_TYPE_FOR_STATEMENT:
    case AST_NODE_TYPE_RETURN_STATEMENT:
    {
        if (array_size(node->children) == 0)
            break;

        AST_node_t *stmt = node->children[array_size(node->children) - 1];
        if (stmt->type == AST_NODE_TYPE_COMPOUND_STATEMENT)
        {
            begin_scope();
            process_AST_node(stmt);
            end_scope();
        }
        else
            process_AST_node(stmt);
        break;
    }

    default:
        ASSERT(false);
        break;
    }
}

void create_scopes(AST_node_t *AST)
{
    ASSERT(AST);
    ASSERT(AST->type == AST_NODE_TYPE_TRANSLATION_UNIT);

    begin_scope();
    for (size_t i = 0; i < array_size(AST->children); i++)
    {
        if (AST->children[i] == NULL)
            continue;

        if (AST->children[i]->type == AST_NODE_TYPE_COMPOUND_STATEMENT)
        {
            begin_scope();
            process_AST_node(AST->children[i]);
            end_scope();
        }
        else
            process_AST_node(AST->children[i]);
    }
    end_scope();
}

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
    if (!compare_types(left.type, right.type))
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
