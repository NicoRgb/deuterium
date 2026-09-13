#pragma once

#include "array.h"
#include "AST.h"
#include "scope.h"
#include "hashtable.h"

typedef enum
{
    IR_TYPE_VOID,
    IR_TYPE_INTEGER,
    IR_TYPE_INTLIT,
    IR_TYPE_ARRAY,
    IR_TYPE_POINTER,
    IR_TYPE_FUNCTION,
} ir_type_kind_t;

typedef struct _ir_type
{
    ir_type_kind_t kind;

    union
    {
        struct
        {
            unsigned bits;
        } integer;

        struct
        {
            struct _ir_type *dest;
        } pointer;

        struct
        {
            struct _ir_type *element;
            size_t size;
        } array;

        struct
        {
            struct _ir_type *return_type;
            static_array_t(struct _ir_type *) parameter_types;
        } function;
    };
} ir_type_t;

typedef enum
{
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_REM,

    IR_LOAD,
    IR_STORE,

    IR_ALLOCA,

    IR_CMP_EQ,
    IR_CMP_NE,
    IR_CMP_LT,
    IR_CMP_LE,
    IR_CMP_GT,
    IR_CMP_GE,

    IR_BR,
    IR_COND_BR,
    IR_CALL,
    IR_RET
} ir_opcode_t;

typedef enum
{
    IR_VALUE_GLOBAL,
    IR_VALUE_PARAMETER,
    IR_VALUE_INSTRUCTION,
} ir_value_kind_t;

typedef uint64_t ir_value_id_t;
typedef uint64_t ir_block_id_t;
typedef uint64_t ir_inst_id_t;

typedef struct ir_value
{
    uint64_t id;
    ir_value_kind_t kind;
    ir_type_t *type;
} ir_value_t;

typedef struct
{
    ir_value_t value;
} ir_global_t;

typedef struct
{
    ir_value_t value;

    size_t index;
} ir_parameter_t;

typedef struct
{
    struct _ir_function *function;

    array_t(struct _ir_inst *) instructions;
    struct _ir_inst *terminator;
} ir_block_t;

typedef struct _ir_inst
{
    ir_value_t value;

    ir_opcode_t opcode;

    union
    {
        struct
        {
            ir_value_t *lhs;
            ir_value_t *rhs;
        } binary;

        struct
        {
            ir_value_t *address;
        } load;

        struct
        {
            ir_value_t *address;
            ir_value_t *value;
        } store;

        struct
        {
            ir_block_t *target;
        } br;

        struct
        {
            ir_value_t *condition;
            ir_block_t *true_block;
            ir_block_t *false_block;
        } cond_br;

        struct
        {
            ir_value_t *value;
        } ret;

        struct
        {
            ir_value_t *callee;
            array_t(ir_value_t *) arguments;
        } call;
    };
} ir_inst_t;

typedef struct
{
    hashtable_t table;
} symbol_ir_map_t;

typedef struct _ir_function
{
    const char *name;
    ir_type_t *function_type;

    array_t(ir_parameter_t *) parameters;
    array_t(ir_block_t *) blocks;

    symbol_ir_map_t bindings;
} ir_function_t;

typedef struct
{
    array_t(ir_function_t *) functions;
    array_t(ir_global_t *) globals;

    symbol_ir_map_t bindings;
} ir_module_t;

ir_module_t *generate_high_level_ir(AST_node_t *ast, scope_t *scope);
