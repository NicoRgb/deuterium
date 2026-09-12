#include "scope.h"

#include <string.h>

scope_t *global_scope = NULL;
scope_t *current_scope = NULL;

type_t *create_type(void)
{
    type_t *res = malloc(sizeof(type_t));
    if (!res)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    return res;
}

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

type_t *clone_type(type_t *t)
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

bool compare_types(type_t *left, type_t *right)
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

static void free_scope(scope_t *scope)
{
    ASSERT(scope);

    for (size_t i = 0; i < array_size(scope->children); i++)
        free_scope(scope->children[i]);

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

symbol_t *resolve_symbol(const char *identifier)
{
    if (!current_scope)
        return NULL;

    scope_t *scope = current_scope;
    while (scope)
    {
        for (size_t i = 0; i < array_size(scope->symbols); i++)
        {
            if (strcmp(scope->symbols[i].name, identifier) == 0)
                return &scope->symbols[i];
        }
        scope = scope->parent;
    }

    printf("end\n");
    return NULL;
}
