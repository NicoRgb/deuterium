#include "hlir.h"
#include "printer.h"

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

    ht_insert(&block->function->bindings.table, sym->id, &instruction->value);
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

static ir_value_t *generate_binary_operation(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);
    ASSERT(node->type == AST_NODE_TYPE_BINARY_OPERATION);

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_PLUS:
    case TOKTYPE_MINUS:
    case TOKTYPE_STAR:
    case TOKTYPE_SLASH:
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
        case TOKTYPE_MINUS:
            instruction->opcode = IR_SUB;
            break;
        case TOKTYPE_STAR:
            instruction->opcode = IR_MUL;
            break;
        case TOKTYPE_SLASH:
            instruction->opcode = IR_DIV;
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
    case TOKTYPE_EQUAL_EQUAL:
    case TOKTYPE_NOT_EQUAL:
    case TOKTYPE_LESS:
    case TOKTYPE_LESS_EQUAL:
    case TOKTYPE_GREATER:
    case TOKTYPE_GREATER_EQUAL:
    {
        ir_value_t *left = generate_expression(node->children[0], block);
        ir_value_t *right = generate_expression(node->children[1], block);

        ir_inst_t *cmp = ir_alloc(sizeof(ir_inst_t));

        switch (node->tokens[0]->type)
        {
        case TOKTYPE_EQUAL_EQUAL:
            cmp->opcode = IR_CMP_EQ;
            break;
        case TOKTYPE_NOT_EQUAL:
            cmp->opcode = IR_CMP_NE;
            break;
        case TOKTYPE_LESS:
            cmp->opcode = IR_CMP_LT;
            break;
        case TOKTYPE_LESS_EQUAL:
            cmp->opcode = IR_CMP_LE;
            break;
        case TOKTYPE_GREATER:
            cmp->opcode = IR_CMP_GT;
            break;
        case TOKTYPE_GREATER_EQUAL:
            cmp->opcode = IR_CMP_GE;
            break;
        default:
            ASSERT(0);
        }

        cmp->binary.lhs = left;
        cmp->binary.rhs = right;

        ir_type_t *type = ir_alloc(sizeof(ir_type_t));
        type->kind = IR_TYPE_INTEGER;
        type->integer.bits = 1;

        cmp->value.id = ir_generate_id();
        cmp->value.kind = IR_VALUE_INSTRUCTION;
        cmp->value.type = type;

        array_push(block->instructions, cmp);

        return &cmp->value;
    }

    default:
        ASSERT(0);
    }
}

static ir_value_t *generate_unary_operation(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);
    ASSERT(node->type == AST_NODE_TYPE_UNARY_OPERATION);

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_AMPERSAND:
    case TOKTYPE_STAR:
    case TOKTYPE_TILDE:
    case TOKTYPE_BANG:
        ASSERT(0); // TODO

    default:
        ASSERT(0);
    }

    return NULL;
}

static ir_value_t *generate_postfix_operation(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);

    ir_value_t *val = generate_expression(node->children[0], block);
    ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
    instruction->value.id = ir_generate_id();
    instruction->value.kind = IR_VALUE_INSTRUCTION;
    instruction->value.type = val->type;

    switch (node->tokens[0]->type)
    {
    case TOKTYPE_PLUS_PLUS:
        instruction->opcode = IR_ADD;
        break;
    case TOKTYPE_MINUS_MINUS:
        instruction->opcode = IR_SUB;
        break;
    default:
        ASSERT(0);
    }
    instruction->binary.lhs = val;
    instruction->binary.rhs = &make_integer_constant(1)->base;

    ir_value_t *new_val = &instruction->value;
    array_push(block->instructions, instruction);

    ASSERT(node->children[0]->type == AST_NODE_TYPE_IDENTIFIER);
    uint64_t sym_id = node->children[0]->symbol->id;
    ir_value_t *addr = (ir_value_t *)ht_get(&block->function->bindings.table, sym_id);
    if (!addr)
        addr = (ir_value_t *)ht_get(&block->function->module->bindings.table, sym_id);

    instruction = ir_alloc(sizeof(ir_inst_t));
    instruction->value.id = ir_generate_id();
    instruction->value.kind = IR_VALUE_INSTRUCTION;
    instruction->value.type = addr->type;
    instruction->opcode = IR_STORE;

    instruction->store.address = addr;
    instruction->store.value = new_val;

    array_push(block->instructions, instruction);

    return val;
}

static ir_value_t *generate_member_access(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);

    ASSERT(0);
    return NULL;
}

static ir_value_t *generate_pointer_member_access(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);

    ASSERT(0);
    return NULL;
}

static ir_value_t *generate_array_subscript(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);

    ASSERT(0);
    return NULL;
}

static ir_value_t *generate_function_call(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);

    ASSERT(0);
    return NULL;
}

static ir_value_t *generate_assignment(AST_node_t *node, ir_block_t *block)
{
    ASSERT(node);
    ASSERT(block);
    ASSERT(node->tokens[0]->type == TOKTYPE_EQUAL);

    uint64_t sym_id = node->children[0]->symbol->id;
    ir_value_t *left = (ir_value_t *)ht_get(&block->function->bindings.table, sym_id);
    if (!left)
        left = (ir_value_t *)ht_get(&block->function->module->bindings.table, sym_id);

    ir_value_t *right = generate_expression(node->children[1], block);

    ir_inst_t *instruction = ir_alloc(sizeof(ir_inst_t));
    instruction->value.id = ir_generate_id();
    instruction->value.kind = IR_VALUE_INSTRUCTION;
    instruction->value.type = left->type;
    instruction->opcode = IR_STORE;

    instruction->store.address = left;
    instruction->store.value = right;

    array_push(block->instructions, instruction);
    return right;
}

static ir_value_t *generate_expression(AST_node_t *expr, ir_block_t *block)
{
    ASSERT(expr);
    ASSERT(block);

    switch (expr->type)
    {
    case AST_NODE_TYPE_BINARY_OPERATION:
        return generate_binary_operation(expr, block);

    case AST_NODE_TYPE_UNARY_OPERATION:
        return generate_unary_operation(expr, block);

    case AST_NODE_TYPE_POSTFIX_OPERATION:
        return generate_postfix_operation(expr, block);

    case AST_NODE_TYPE_MEMBER_ACCESS:
        return generate_member_access(expr, block);

    case AST_NODE_TYPE_POINTER_MEMBER_ACCESS:
        return generate_pointer_member_access(expr, block);

    case AST_NODE_TYPE_ARRAY_SUBSCRIPT:
        return generate_array_subscript(expr, block);

    case AST_NODE_TYPE_FUNCTION_CALL:
        return generate_function_call(expr, block);

    case AST_NODE_TYPE_ASSIGNMENT:
        return generate_assignment(expr, block);

    case AST_NODE_TYPE_IDENTIFIER:
    {
        ir_value_t *value = (ir_value_t *)ht_get(&block->function->bindings.table, expr->symbol->id);
        if (!value)
            value = (ir_value_t *)ht_get(&block->function->module->bindings.table, expr->symbol->id);

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
        ASSERT_MSG(0, "type: %s", AST_node_type_to_string(expr->type));
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

static ir_value_t *generate_boolean_from_condition(AST_node_t *expr, ir_block_t *block)
{
    ir_value_t *condition = generate_expression(expr, block);
    if (condition->type->kind != IR_TYPE_INTEGER || condition->type->integer.bits != 1)
    {
        switch (condition->type->kind)
        {
        case IR_TYPE_INTEGER:
        case IR_TYPE_INTLIT:
        case IR_TYPE_POINTER:
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

    return condition;
}

static ir_block_t *generate_if_statement(AST_node_t *statement, ir_block_t *block)
{
    ASSERT(statement);
    ASSERT(block);
    ASSERT(statement->type == AST_NODE_TYPE_IF_STATEMENT);

    ir_value_t *condition = generate_boolean_from_condition(statement->children[0], block);

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

static ir_block_t *generate_while_statement(AST_node_t *statement, ir_block_t *block)
{
    ASSERT(statement);
    ASSERT(block);
    ASSERT(statement->type == AST_NODE_TYPE_WHILE_STATEMENT);

    ir_block_t *loop_block = ir_alloc(sizeof(ir_block_t));
    loop_block->id = ir_generate_id();
    loop_block->function = block->function;
    loop_block->terminator = NULL;
    array_create(ir_inst_t *, loop_block->instructions);
    array_push(block->function->blocks, loop_block);

    ir_block_t *exit_block = ir_alloc(sizeof(ir_block_t));
    exit_block->id = ir_generate_id();
    exit_block->function = block->function;
    exit_block->terminator = NULL;
    array_create(ir_inst_t *, exit_block->instructions);
    array_push(block->function->blocks, exit_block);

    ir_value_t *condition = generate_boolean_from_condition(statement->children[0], block);

    ir_inst_t *cond_br = ir_alloc(sizeof(ir_inst_t));
    cond_br->opcode = IR_COND_BR;
    cond_br->cond_br.condition = condition;
    cond_br->cond_br.true_block = loop_block;
    cond_br->cond_br.false_block = exit_block;

    block->terminator = cond_br;

    generate_statement(statement->children[1], loop_block);
    condition = generate_boolean_from_condition(statement->children[0], loop_block);

    cond_br = ir_alloc(sizeof(ir_inst_t));
    cond_br->opcode = IR_COND_BR;
    cond_br->cond_br.condition = condition;
    cond_br->cond_br.true_block = loop_block;
    cond_br->cond_br.false_block = exit_block;

    loop_block->terminator = cond_br;

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
            generate_variable(current_node->symbol, current_block);
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
    case AST_NODE_TYPE_WHILE_STATEMENT:
    {
        return generate_while_statement(node, block);
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
        ASSERT_MSG(0, "type: %s", AST_node_type_to_string(node->type));
    }

    return NULL;
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

            ht_insert(&function->bindings.table, sym.id, &param->value);
            break;
        }

        case SYMBOL_VARIABLE:
        {
            break;
        }

        case SYMBOL_FUNCTION:
        default:
            ASSERT(0);
        }
    }

    AST_node_t *compound_statement = ast->children[0];
    generate_compound_statement(compound_statement, entry);

    return function;
}

ir_module_t *generate_high_level_ir(scope_t *scope)
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

            ht_insert(&module->bindings.table, sym.id, &global->value);
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
