#include "sema.h"
#include "strdup.h"

static size_t max(size_t a, size_t b)
{
    if (a > b)
        return a;
    return b;
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
        if (array_size(node->children) > 0)
        {
            type_t *type = check_expression(node->children[0]);
            if (type)
                free_type(type);
        }
        break;

    case AST_NODE_TYPE_IF_STATEMENT:
    case AST_NODE_TYPE_WHILE_STATEMENT:
    case AST_NODE_TYPE_FOR_STATEMENT:
    case AST_NODE_TYPE_RETURN_STATEMENT: // TODO: check expressions
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
