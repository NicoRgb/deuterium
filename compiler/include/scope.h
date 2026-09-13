#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "array.h"
#include "AST.h"

typedef enum
{
    TYPE_VOID,
    TYPE_INT,

    TYPE_INTLIT,

    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION,
} type_kind_t;

typedef struct _type
{
    type_kind_t kind;

    union
    {
        struct
        {
            struct _type *dest;
        } pointer;

        struct
        {
            struct _type *element;
            size_t size;
        } array;

        struct
        {
            struct _type *return_type;
            static_array_t(struct _type *) parameter_types;
        } function;
    };
} type_t;

typedef struct
{
    bool is_valid;
    int64_t val;
} symbol_const_value_t;

typedef enum
{
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER
} symbol_kind_t;

typedef struct
{
    symbol_kind_t kind;
    char *name;
    type_t *type;
    AST_node_t *declaration;

    symbol_const_value_t value;
} symbol_t;

typedef struct _scope
{
    struct _scope *parent;
    array_t(struct _scope *) children;

    array_t(symbol_t) symbols;

    AST_node_t *declaration;
} scope_t;

type_t *create_type(void);
void free_type(type_t *type);
type_t *clone_type(type_t *t);
bool compare_types(type_t *left, type_t *right);

void free_scopes(void);

scope_t *begin_scope(AST_node_t *declaration);
void end_scope(void);
void symbol_insert(symbol_t symbol);

scope_t *get_global_scope(void);
scope_t *get_current_scope(void);

scope_t *get_scope_by_function(symbol_t *symbol);
symbol_t *resolve_symbol(const char *identifier);
