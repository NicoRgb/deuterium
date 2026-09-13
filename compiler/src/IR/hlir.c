#include "hlir.h"

#include "external/arena.h"

static Arena hlir_arena = {0};

static void *ir_alloc(size_t size)
{
    void *res = arena_alloc(&hlir_arena, size);
    if (!res)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    return res;
}

static ir_type_kind_t type_kind_to_ir_kind_type(type_kind_t kind)
{
    switch (kind)
    {
    case TYPE_VOID:
        return IR_TYPE_VOID;
    case TYPE_INT:
        return IR_TYPE_INTEGER;
    case TYPE_INTLIT:
        return IR_TYPE_INTLIT;
    case TYPE_ARRAY:
        return IR_TYPE_ARRAY;
    case TYPE_POINTER:
        return IR_TYPE_POINTER;
    case TYPE_FUNCTION:
        return IR_TYPE_FUNCTION;
    default:
        ASSERT(0);
    }
}

static ir_type_t *type_to_ir_type(type_t *type)
{
    ir_type_t *res = ir_alloc(sizeof(ir_type_t));
    res->kind = type_kind_to_ir_kind_type(type->kind);

    switch (res->kind)
    {
    case IR_TYPE_INTEGER:
        res->integer.bits = 32;
        break;

    case IR_TYPE_POINTER:
        res->pointer.dest = type_to_ir_type(type->pointer.dest);
        break;

    case IR_TYPE_ARRAY:
        res->array.size = type->array.size;
        res->array.element = type_to_ir_type(type->array.element);
        break;

    case IR_TYPE_FUNCTION:
        res->function.return_type = type_to_ir_type(type->function.return_type);
        static_array_create(ir_type_t *, array_size(type->function.parameter_types), res->function.parameter_types);

        type_t *it;
        foreach (type->function.parameter_types, it)
            static_array_push(res->function.parameter_types, type_to_ir_type(it));

        break;
    }

    return res;
}

uint64_t g_current_id = 0;
static uint64_t ir_generate_id(void)
{
    return g_current_id++;
}

static ir_value_t *generate_expression(AST_node_t *expr, ir_block_t *block);
static ir_value_t *generate_binary_operation(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);
    ASSERT(node->type == AST_NODE_TYPE_BINARY_OPERATION);

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_PLUS:
    {
        ir_value_t *left = generate_expression(node->children[0], block);
        ir_value_t *right = generate_expression(node->children[1], block);

        ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
        instruction->value.id = ir_generate_id();
        instruction->value.kind = IR_VALUE_INSTRUCTION;
        instruction->value.type = left->type;

        instruction->opcode = IR_ADD;

        instruction->binary.lhs = left;
        instruction->binary.rhs = right;

        array_push(block->instructions, instruction);

        return &instruction->value;
    }

    default:
        ASSERT(0);
    }
}

static ir_value_t *generate_expression(AST_node_t *expr, ir_block_t *block)
{
    ASSERT(expr);
    ASSERT(block);

    switch (expr->type)
    {
    case AST_NODE_TYPE_BINARY_OPERATION:
        return generate_binary_operation(expr, block);

    case AST_NODE_TYPE_IDENTIFIER:
        return (ir_value_t *)hashtable_get(&block->function->bindings.table, expr->tokens[0]->text);

    default:
        ASSERT(0);
    }
}

static void generate_return_statement(AST_node_t *statement, ir_block_t *block)
{
    ASSERT(statement);
    ASSERT(block);
    ASSERT(statement->type == AST_NODE_TYPE_RETURN_STATEMENT);

    ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
    instruction->opcode = IR_RET;

    instruction->ret.value = NULL;
    if (array_size(statement->children) > 0)
    {
        ir_value_t *val = generate_expression(statement->children[0], block);
        instruction->ret.value = val;
    }

    array_push(block->instructions, instruction);
}

static ir_function_t *generate_function(symbol_t *symbol, AST_node_t *ast, scope_t *scope)
{
    ASSERT(symbol);
    ASSERT(ast);
    ASSERT(scope);

    ir_function_t *function = ir_alloc(sizeof(ir_function_t));
    function->function_type = type_to_ir_type(symbol->type);
    function->name = symbol->name;
    array_create(ir_block_t *, function->blocks);
    array_create(ir_parameter_t *, function->parameters);
    hashtable_create(&function->bindings.table);

    size_t param_index = 0;
    symbol_t sym;
    foreach (scope->symbols, sym)
    {
        switch (sym.kind)
        {
        case SYMBOL_VARIABLE:
            // TODO
            break;

        case SYMBOL_PARAMETER:
        {
            ir_parameter_t *param = ir_alloc(sizeof(ir_parameter_t));
            param->index = param_index++;
            param->value.id = ir_generate_id();
            param->value.kind = IR_VALUE_PARAMETER;
            param->value.type = type_to_ir_type(sym.type);
            array_push(function->parameters, param);

            hashtable_insert(&function->bindings.table, sym.name, &param->value);
            break;
        }

        case SYMBOL_FUNCTION:
        default:
            ASSERT(0);
        }
    }

    AST_node_t *compound_statement = ast->children[2];
    ir_block_t *entry = ir_alloc(sizeof(ir_block_t));
    entry->function = function;
    entry->terminator = NULL;
    array_create(ir_inst_t *, entry->instructions);

    array_push(function->blocks, entry);

    AST_node_t *current_node;
    foreach (compound_statement->children, current_node)
    {
        switch (current_node->type)
        {
        case AST_NODE_TYPE_RETURN_STATEMENT:
        {
            generate_return_statement(current_node, entry);
            break;
        }

        default:
            ASSERT(0);
        }
    }

    return function;
}

ir_module_t *generate_high_level_ir(AST_node_t *ast, scope_t *scope)
{
    ir_module_t *module = ir_alloc(sizeof(ir_module_t));
    array_create(ir_function_t *, module->functions);
    array_create(ir_global_t *, module->globals);
    hashtable_create(&module->bindings.table);

    symbol_t sym;
    foreach (scope->symbols, sym)
    {
        switch (sym.kind)
        {
        case SYMBOL_VARIABLE:
            // TODO: globals
            break;

        case SYMBOL_FUNCTION:
            break;

        case SYMBOL_PARAMETER:
        default:
            ASSERT(0);
        }
    }

    foreach (scope->symbols, sym)
    {
        switch (sym.kind)
        {
        case SYMBOL_VARIABLE:
            break;

        case SYMBOL_FUNCTION:
        {
            scope_t *func_scope = get_scope_by_function(&sym);
            ASSERT(func_scope);
            array_push(module->functions, generate_function(&sym, sym.declaration, func_scope));
            break;
        }

        case SYMBOL_PARAMETER:
        default:
            ASSERT(0);
        }
    }

    return module;
}
