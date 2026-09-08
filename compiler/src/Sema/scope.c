#include "scope.h"

scope_t *global_scope = NULL;
scope_t *current_scope = NULL;

void free_type(type_t *type)
{
    ASSERT(type);

    switch (type->kind)
    {
    case TYPE_ARRAY:
        free_type(type->array.element);
        break;

    case TYPE_POINTER:
        free_type(type->pointer.dest);
        break;

    case TYPE_FUNCTION:
        free_type(type->function.return_type);

        if (type->function.parameter_types == NULL_ARRAY)
            break;

        for (size_t i = 0; i < static_array_size(type->function.parameter_types); i++)
            free_type(type->function.parameter_types[i]);

        static_array_free(type->function.parameter_types);
        break;

    default:
        break;
    }

    free(type);
}

static void free_scope(scope_t *scope)
{
    ASSERT(scope);

    for (size_t i = 0; i < array_size(scope->children); i++)
        free_scope(scope);

    for (size_t i = 0; i < array_size(scope->symbols); i++)
    {
        symbol_t *sym = &(scope->symbols[i]);
        free(sym->name);
        free_type(sym->type);
    }

    array_free(scope->children);
    array_free(scope->symbols);
}

void free_scopes(void)
{
    if (global_scope)
        free_scope(global_scope);

    global_scope = NULL;
    current_scope = NULL;
}

static scope_t *scope_create(void)
{
    scope_t *s = malloc(sizeof(scope_t));
    if (!s)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    s->parent = NULL;
    array_create(scope_t *, s->children);
    array_create(symbol_t, s->symbols);

    return s;
}

scope_t *begin_scope(void)
{
    scope_t *scope = scope_create();
    scope->parent = current_scope;

    if (current_scope)
        array_push(current_scope->children, scope);

    current_scope = scope;
    if (!global_scope)
        global_scope = current_scope;

    return current_scope;
}

void end_scope(void)
{
    if (current_scope->parent == NULL)
    {
        current_scope = global_scope;
        return;
    }

    current_scope = current_scope->parent;
}

void symbol_insert(symbol_t symbol)
{
    if (!current_scope)
        return;

    array_push(current_scope->symbols, symbol);
}

scope_t *get_global_scope(void)
{
    return global_scope;
}
