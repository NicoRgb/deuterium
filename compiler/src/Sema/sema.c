#include "sema.h"
#include "strdup.h"

static size_t max(size_t a, size_t b)
{
    if (a > b)
        return a;
    return b;
}

uint64_t g_current_symbol_id = 0;
static uint64_t generate_symbol_id(void)
{
    return g_current_symbol_id++;
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
            if (!size_val.is_valid)
            {
                // TODO: VAR?

                free_type(type);
                push_error(&current_node->start, &current_node->end, "invalid size for array");
                return NULL;
            }
            t->array.size = size_val.val;

            type = t;
        }

        current_node = current_node->children[0];
    }

    if (out_node)
        *out_node = current_node;

    return type;
}

const char *identifier_from_node(AST_node_t *node)
{
    ASSERT(node);

    AST_node_t *current_node = node->children[1];

    while (current_node->type == AST_NODE_TYPE_POINTER_TYPE || current_node->type == AST_NODE_TYPE_ARRAY_TYPE || current_node->type == AST_NODE_TYPE_FUNCTION_TYPE || current_node->type == AST_NODE_TYPE_BINARY_OPERATION)
    {
        current_node = current_node->children[0];
    }

    ASSERT(current_node->type == AST_NODE_TYPE_DECLARATOR);

    return current_node->tokens[0]->text;
}

static void process_declaration(AST_node_t *declaration)
{
    ASSERT(declaration);

    symbol_t sym;
    sym.id = generate_symbol_id();
    sym.kind = SYMBOL_VARIABLE;
    sym.declaration = declaration;

    AST_node_t *func_type_or_declarator;
    sym.type = process_type(declaration, &func_type_or_declarator);

    symbol_const_value_t sym_val;
    sym_val.is_valid = true;
    sym_val.val = 0;
    if (declaration->children[1]->type == AST_NODE_TYPE_BINARY_OPERATION)
    {
        sym_val = eval_constant_expression(declaration->children[1]->children[1]);
    }

    if (!sym.type)
    {
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

    if (!sym_val.is_valid)
    {
        free_type(sym.type);
        push_error(&declaration->start, &declaration->end, "type error");

        return;
    }

    sym.value.is_valid = false;
    if (sym.kind == SYMBOL_VARIABLE && (sym.type->kind == TYPE_INT || sym.type->kind == TYPE_POINTER))
    {
        sym.value = sym_val;
    }

    ASSERT(func_type_or_declarator->type == AST_NODE_TYPE_DECLARATOR);
    ASSERT(func_type_or_declarator->tokens[0]->type == TOKTYPE_IDENTIFIER);
    sym.name = strdup(func_type_or_declarator->tokens[0]->text);

    symbol_insert(sym);

    free_AST_node(declaration->children[0]);
    free_AST_node(declaration->children[1]);
    array_clear(declaration->children);

    declaration->symbol = resolve_symbol(sym.name);
}

static void process_parameter(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(node->type == AST_NODE_TYPE_PARAMETER);

    symbol_t sym;
    sym.id = generate_symbol_id();
    sym.kind = SYMBOL_PARAMETER;
    sym.declaration = node;
    sym.value.is_valid = false;

    AST_node_t *declarator;
    sym.type = process_type(node, &declarator);

    ASSERT(declarator->type == AST_NODE_TYPE_DECLARATOR);
    ASSERT(declarator->tokens[0]->type == TOKTYPE_IDENTIFIER);
    sym.name = strdup(declarator->tokens[0]->text);

    symbol_insert(sym);

    free_AST_node(node->children[0]);
    free_AST_node(node->children[1]);
    array_clear(node->children);

    node->symbol = resolve_symbol(sym.name);
}

static void process_AST_node(AST_node_t *node);
static void process_function_definition(AST_node_t *node)
{
    ASSERT(node);
    ASSERT(array_size(node->children) >= 1);

    symbol_t sym;
    sym.id = generate_symbol_id();
    sym.kind = SYMBOL_FUNCTION;
    sym.declaration = node;
    sym.value.is_valid = false;

    AST_node_t *func_type = node->children[1];
    ASSERT(array_size(func_type->children) >= 1);

    type_t *t = create_type();
    t->kind = TYPE_FUNCTION;
    t->function.return_type = process_simple_type(node->children[0]);
    static_array_create(type_t *, max(array_size(func_type->children) - 1, 1), t->function.parameter_types);

    sym.name = strdup(func_type->children[0]->tokens[0]->text);
    sym.type = t;

    symbol_insert(sym);

    begin_scope(node);
    for (size_t i = 1; i < array_size(func_type->children); i++)
    {
        array_push(t->function.parameter_types, process_type(func_type->children[i], NULL));
        process_parameter(func_type->children[i]);
    }
    process_AST_node(node->children[2]);
    end_scope();

    free_AST_node(node->children[0]);
    free_AST_node(node->children[1]);
    node->children[0] = node->children[2];
    _array_set_size(node->children, 1);

    node->symbol = resolve_symbol(sym.name); // this is necessary because the array own the copies the symbol
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
            begin_scope(child);
            process_AST_node(child);
            end_scope();
        }
        else
            process_AST_node(child);
    }
}

static void process_statement_child(AST_node_t *node, size_t child_index)
{
    ASSERT(array_size(node->children) > child_index);

    AST_node_t *stmt = node->children[child_index];
    if (stmt->type == AST_NODE_TYPE_COMPOUND_STATEMENT)
    {
        begin_scope(node);
        process_AST_node(stmt);
        end_scope();
    }
    else
        process_AST_node(stmt);
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
        if (array_size(node->children) > 0)
        {
            type_t *type = check_expression(node->children[0]);
            if (type)
                free_type(type);
        }
        break;

    case AST_NODE_TYPE_WHILE_STATEMENT:
    case AST_NODE_TYPE_IF_STATEMENT:
    {
        type_t *type = check_expression(node->children[0]);
        if (type)
            free_type(type);

        process_statement_child(node, 1);
        if (array_size(node->children) > 2)
            process_statement_child(node, 2);

        break;
    }

    case AST_NODE_TYPE_FOR_STATEMENT:
    {
        begin_scope(node);
        process_AST_node(node->children[0]);
        process_AST_node(node->children[1]);

        if (array_size(node->children) > 3)
        {
            type_t *type = check_expression(node->children[2]);
            if (type)
                free_type(type);

            process_AST_node(node->children[3]);
        }
        else
            process_AST_node(node->children[2]);

        end_scope();
        break;
    }

    case AST_NODE_TYPE_RETURN_STATEMENT:
    {
        scope_t *scope = get_current_scope();
        if (!scope)
        {
            ASSERT(0);
            break;
        }

        scope_t *current_scope = scope;
        for (; current_scope != NULL; current_scope = current_scope->parent)
        {
            if (scope->declaration->type == AST_NODE_TYPE_FUNCTION_DEFINITION)
                break;
        }

        if (!current_scope)
        {
            ASSERT(0);
            break;
        }

        type_t *type = NULL;
        if (array_size(node->children) > 0)
        {
            type = check_expression(node->children[0]);
            if (!type)
                break;
        }

        const char *identifier = identifier_from_node(current_scope->declaration);
        symbol_t *func = resolve_symbol(identifier);
        ASSERT(func);
        ASSERT(func->kind == SYMBOL_FUNCTION);

        if (!(type && compare_types(type, func->type->function.return_type)) && !(!type && func->type->function.return_type->kind == TYPE_VOID))
            push_error(&node->start, &node->end, "invalid return type");

        if (type)
            free_type(type);
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

    begin_scope(AST);
    for (size_t i = 0; i < array_size(AST->children); i++)
    {
        if (AST->children[i] == NULL)
            continue;

        if (AST->children[i]->type == AST_NODE_TYPE_COMPOUND_STATEMENT)
        {
            begin_scope(AST->children[i]);
            process_AST_node(AST->children[i]);
            end_scope();
        }
        else
            process_AST_node(AST->children[i]);
    }
    end_scope();
}
