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

    default:
        ASSERT(0);
    }

    return res;
}

uint64_t g_current_id = 0;
static uint64_t ir_generate_id(void)
{
    return g_current_id++;
}

static ir_constant_t *make_integer_constant(uint64_t value)
{
    ir_constant_t *constant = ir_alloc(sizeof(ir_constant_t));
    ir_type_t *type = ir_alloc(sizeof(ir_type_t));
    type->kind = IR_TYPE_INTLIT;

    constant->base.kind = IR_VALUE_CONSTANT;
    constant->base.id = ir_generate_id();
    constant->base.type = type;
    constant->value = value;

    return constant;
}

static ir_block_t *generate_statement(AST_node_t *node, ir_block_t *block);
static ir_value_t *generate_expression(AST_node_t *expr, ir_block_t *block);

static ir_value_t *generate_binary_operation(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);
    ASSERT(node->type == AST_NODE_TYPE_BINARY_OPERATION);

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_PLUS:
    case TOKTYPE_PERCENT:
    {
        ir_value_t *left = generate_expression(node->children[0], block);
        ir_value_t *right = generate_expression(node->children[1], block);

        ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
        instruction->value.id = ir_generate_id();
        instruction->value.kind = IR_VALUE_INSTRUCTION;
        instruction->value.type = left->type;

        switch (node->tokens[0]->type)
        {
        case TOKTYPE_PLUS:
            instruction->opcode = IR_ADD;
            break;
        case TOKTYPE_PERCENT:
            instruction->opcode = IR_REM;
            break;
        default:
            ASSERT(0);
        }

        instruction->binary.lhs = left;
        instruction->binary.rhs = right;

        array_push(block->instructions, instruction);

        return &instruction->value;
    }

    default:
        ASSERT(0);
    }
}

static ir_value_t *generate_assignment(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);

    const char *identifer = node->children[0]->tokens[0]->text;
    ir_value_t *left = (ir_value_t *)hashtable_get(&block->function->bindings.table, identifer);
    if (!left)
        left = (ir_value_t *)hashtable_get(&block->function->module->bindings.table, identifer);

    ir_value_t *right = generate_expression(node->children[1], block);

    ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
    instruction->value.id = ir_generate_id();
    instruction->value.kind = IR_VALUE_INSTRUCTION;
    instruction->value.type = left->type;

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_EQUAL:
        instruction->opcode = IR_STORE;
        break;

    default:
        ASSERT(0);
    }

    instruction->store.address = left;
    instruction->store.value = right;

    array_push(block->instructions, instruction);
    return &instruction->value;
}

static ir_value_t *generate_expression(AST_node_t *expr, ir_block_t *block)
{
    ASSERT(expr);
    ASSERT(block);

    switch (expr->type)
    {
    case AST_NODE_TYPE_BINARY_OPERATION:
        return generate_binary_operation(expr, block);

    case AST_NODE_TYPE_ASSIGNMENT:
        return generate_assignment(expr, block);

    case AST_NODE_TYPE_IDENTIFIER:
    {
        ir_value_t *value = (ir_value_t *)hashtable_get(&block->function->bindings.table, expr->tokens[0]->text);
        if (!value)
            value = (ir_value_t *)hashtable_get(&block->function->module->bindings.table, expr->tokens[0]->text);

        ASSERT(value);
        if (value->kind == IR_VALUE_PARAMETER)
            return value;

        ASSERT(value->type->kind == IR_TYPE_POINTER);

        ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
        instruction->value.id = ir_generate_id();
        instruction->value.kind = IR_VALUE_INSTRUCTION;
        instruction->value.type = value->type->pointer.dest;
        instruction->opcode = IR_LOAD;
        instruction->load.address = value;

        array_push(block->instructions, instruction);
        return &instruction->value;
    }

    case AST_NODE_TYPE_INTEGER_LITERAL:
    {
        ir_constant_t *constant = make_integer_constant(atoll(expr->tokens[0]->text));
        return &constant->base;
    }

    default:
        ASSERT(0);
    }
}

static ir_block_t *generate_return_statement(AST_node_t *statement, ir_block_t *block)
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

    block->terminator = instruction;
    return NULL;
}

static ir_block_t *generate_if_statement(AST_node_t *statement, ir_block_t *block)
{
    ASSERT(statement);
    ASSERT(block);
    ASSERT(statement->type == AST_NODE_TYPE_IF_STATEMENT);

    ir_value_t *condition = generate_expression(statement->children[0], block);
    if (condition->type->kind != IR_TYPE_INTEGER || condition->type->integer.bits != 1)
    {
        switch (condition->type->kind)
        {
        case IR_TYPE_INTEGER:
        {
            ir_inst_t *cmp = ir_alloc(sizeof(ir_inst_t));
            cmp->opcode = IR_CMP_NE;
            cmp->binary.lhs = condition;
            cmp->binary.rhs = &make_integer_constant(0)->base;

            ir_type_t *type = ir_alloc(sizeof(ir_type_t));
            type->kind = IR_TYPE_INTEGER;
            type->integer.bits = 1;

            cmp->value.id = ir_generate_id();
            cmp->value.kind = IR_VALUE_INSTRUCTION;
            cmp->value.type = type;

            array_push(block->instructions, cmp);

            condition = &cmp->value;
            break;
        }

        default:
            ASSERT(0);
        }
    }

    ir_block_t *true_block = ir_alloc(sizeof(ir_block_t));
    true_block->id = ir_generate_id();
    true_block->function = block->function;
    true_block->terminator = NULL;
    array_create(ir_inst_t *, true_block->instructions);
    array_push(block->function->blocks, true_block);

    generate_statement(statement->children[1], true_block);

    ir_block_t *false_block = NULL;

    if (array_size(statement->children) > 2)
    {
        false_block = ir_alloc(sizeof(ir_block_t));
        false_block->id = ir_generate_id();
        false_block->function = block->function;
        false_block->terminator = NULL;
        array_create(ir_inst_t *, false_block->instructions);
        array_push(block->function->blocks, false_block);

        generate_statement(statement->children[2], false_block);
    }

    ir_block_t *exit_block = ir_alloc(sizeof(ir_block_t));
    exit_block->id = ir_generate_id();
    exit_block->function = block->function;
    exit_block->terminator = NULL;
    array_create(ir_inst_t *, exit_block->instructions);
    array_push(block->function->blocks, exit_block);

    if (!true_block->terminator)
    {
        ir_inst_t *br = ir_alloc(sizeof(ir_inst_t));
        br->opcode = IR_BR;
        br->br.target = exit_block;

        true_block->terminator = br;
    }

    if (false_block && !false_block->terminator)
    {
        ir_inst_t *br = ir_alloc(sizeof(ir_inst_t));
        br->opcode = IR_BR;
        br->br.target = exit_block;

        false_block->terminator = br;
    }

    ir_inst_t *cond_br = ir_alloc(sizeof(ir_inst_t));
    cond_br->opcode = IR_COND_BR;
    cond_br->cond_br.condition = condition;
    cond_br->cond_br.true_block = true_block;
    cond_br->cond_br.false_block = false_block ? false_block : exit_block;

    block->terminator = cond_br;

    return exit_block;
}

static ir_block_t *generate_compound_statement(AST_node_t *node, ir_block_t *block)
{
    ir_block_t *current_block = block;

    AST_node_t *current_node;
    foreach (node->children, current_node)
    {
        if (current_block == NULL)
            break;

        switch (current_node->type)
        {
        case AST_NODE_TYPE_DECLARATION:
            break;

        default:
            current_block = generate_statement(current_node, current_block);
            break;
        }
    }

    return current_block;
}

static ir_block_t *generate_statement(AST_node_t *node, ir_block_t *block)
{
    switch (node->type)
    {
    case AST_NODE_TYPE_RETURN_STATEMENT:
    {
        return generate_return_statement(node, block);
    }
    case AST_NODE_TYPE_IF_STATEMENT:
    {
        return generate_if_statement(node, block);
    }
    case AST_NODE_TYPE_COMPOUND_STATEMENT:
    {
        return generate_compound_statement(node, block);
    }
    case AST_NODE_TYPE_EXPRESSION_STATEMENT:
    {
        generate_expression(node->children[0], block);
        return block;
    }

    default:
        ASSERT(0);
    }

    return NULL;
}

static void generate_variable(symbol_t *sym, ir_block_t *block)
{
    ASSERT(sym);
    ASSERT(block);

    ir_type_t *ptr_type = ir_alloc(sizeof(ir_type_t));
    ptr_type->kind = IR_TYPE_POINTER;
    ptr_type->pointer.dest = type_to_ir_type(sym->type);

    ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
    instruction->value.id = ir_generate_id();
    instruction->value.kind = IR_VALUE_INSTRUCTION;
    instruction->value.type = ptr_type;
    instruction->opcode = IR_ALLOCA;

    ir_value_t *var = &instruction->value;

    hashtable_insert(&block->function->bindings.table, sym->name, &instruction->value);
    array_push(block->instructions, instruction);

    if (ptr_type->pointer.dest->kind == IR_TYPE_INTEGER)
    {
        instruction = ir_alloc(sizeof(ir_inst_t));
        instruction->opcode = IR_STORE;
        instruction->store.address = var;
        instruction->store.value = &make_integer_constant(sym->value.val)->base;
        array_push(block->instructions, instruction);
    }
}

static ir_function_t *generate_function(symbol_t *symbol, AST_node_t *ast, scope_t *scope, ir_module_t *module)
{
    ASSERT(symbol);
    ASSERT(ast);
    ASSERT(scope);

    ir_function_t *function = ir_alloc(sizeof(ir_function_t));
    function->function_type = type_to_ir_type(symbol->type);
    function->name = symbol->name;
    function->module = module;
    array_create(ir_parameter_t *, function->parameters);
    array_create(ir_block_t *, function->blocks);
    hashtable_create(&function->bindings.table);

    ir_block_t *entry = ir_alloc(sizeof(ir_block_t));
    entry->id = ir_generate_id();
    entry->function = function;
    entry->terminator = NULL;
    array_create(ir_inst_t *, entry->instructions);
    array_push(function->blocks, entry);

    size_t param_index = 0;
    symbol_t sym;
    foreach (scope->symbols, sym)
    {
        switch (sym.kind)
        {
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

        case SYMBOL_VARIABLE: // TODO: inner scopes
        {
            generate_variable(&sym, entry);
            break;
        }

        case SYMBOL_FUNCTION:
        default:
            ASSERT(0);
        }
    }

    AST_node_t *compound_statement = ast->children[2];
    generate_compound_statement(compound_statement, entry);

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
        {
            ir_global_t *global = ir_alloc(sizeof(ir_global_t));
            global->value.id = ir_generate_id();
            global->value.kind = IR_VALUE_GLOBAL;
            global->value.type = type_to_ir_type(sym.type);

            switch (global->value.type->kind)
            {
            case IR_TYPE_INTEGER:
                ASSERT(sym.value.is_valid);
                global->initial_value = sym.value.val;
                break;

            default:
                break;
            }
            array_push(module->globals, global);

            hashtable_insert(&module->bindings.table, sym.name, &global->value);
            break;
        }

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
            array_push(module->functions, generate_function(&sym, sym.declaration, func_scope, module));
            break;
        }

        case SYMBOL_PARAMETER:
        default:
            ASSERT(0);
        }
    }

    return module;
}
