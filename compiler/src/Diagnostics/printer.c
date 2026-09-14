#include "parser.h"
#include "logger.h"
#include "printer.h"
#include "ansi.h"

#include <inttypes.h>

#include <stdio.h>
#include <stdbool.h>

const char *AST_node_type_to_string(AST_node_type_t type)
{
    switch (type)
    {
    case AST_NODE_TYPE_TRANSLATION_UNIT:
        return "TRANSLATION_UNIT";

    case AST_NODE_TYPE_FUNCTION_DEFINITION:
        return "FUNCTION_DEFINITION";
    case AST_NODE_TYPE_DECLARATION:
        return "DECLARATION";
    case AST_NODE_TYPE_PARAMETER:
        return "PARAMETER";

    case AST_NODE_TYPE_INIT_DECLARATOR:
        return "AST_NODE_TYPE_INIT_DECLARATOR";
    case AST_NODE_TYPE_BUILTIN_TYPE:
        return "BUILTIN_TYPE";
    case AST_NODE_TYPE_DECLARATOR:
        return "DECLARATOR";
    case AST_NODE_TYPE_POINTER_TYPE:
        return "POINTER_TYPE";
    case AST_NODE_TYPE_FUNCTION_TYPE:
        return "FUNCTION_TYPE";
    case AST_NODE_TYPE_ARRAY_TYPE:
        return "ARRAY_TYPE";

    case AST_NODE_TYPE_COMPOUND_STATEMENT:
        return "COMPOUND_STATEMENT";
    case AST_NODE_TYPE_EXPRESSION_STATEMENT:
        return "EXPRESSION_STATEMENT";
    case AST_NODE_TYPE_IF_STATEMENT:
        return "IF_STATEMENT";
    case AST_NODE_TYPE_WHILE_STATEMENT:
        return "WHILE_STATEMENT";
    case AST_NODE_TYPE_FOR_STATEMENT:
        return "FOR_STATEMENT";
    case AST_NODE_TYPE_RETURN_STATEMENT:
        return "RETURN_STATEMENT";

    case AST_NODE_TYPE_IDENTIFIER:
        return "IDENTIFIER";
    case AST_NODE_TYPE_INTEGER_LITERAL:
        return "INTEGER_LITERAL";

    case AST_NODE_TYPE_UNARY_OPERATION:
        return "UNARY_OPERATION";
    case AST_NODE_TYPE_BINARY_OPERATION:
        return "BINARY_OPERATION";
    case AST_NODE_TYPE_ASSIGNMENT:
        return "ASSIGNMENT";
    case AST_NODE_TYPE_CONDITIONAL_OPERATION:
        return "CONDITIONAL_OPERATION";

    case AST_NODE_TYPE_FUNCTION_CALL:
        return "FUNCTION_CALL";
    case AST_NODE_TYPE_ARRAY_SUBSCRIPT:
        return "ARRAY_SUBSCRIPT";
    case AST_NODE_TYPE_MEMBER_ACCESS:
        return "MEMBER_ACCESS";
    case AST_NODE_TYPE_POINTER_MEMBER_ACCESS:
        return "POINTER_MEMBER_ACCESS";
    case AST_NODE_TYPE_POSTFIX_OPERATION:
        return "POSTFIX_OPERATION";

    default:
        return "UNKNOWN";
    }
}

static void print_indent(const bool *has_sibling, size_t depth)
{
    if (depth == 0)
        return;

    for (size_t i = 0; i + 1 < depth; ++i)
        printf("%s", has_sibling[i] ? "│   " : "    ");

    printf("%s", has_sibling[depth - 1] ? "├── " : "└── ");
}

static void print_simple_indent(size_t depth)
{
    for (size_t i = 0; i < depth; i++)
        printf(" ");
}

static void print_tokens(const AST_node_t *node)
{
    size_t count = array_size(node->tokens);

    if (count == 0)
        return;

    printf(" [");

    for (size_t i = 0; i < count; ++i)
    {
        if (i != 0)
            printf(", ");

        token_t *tok = node->tokens[i];
        printf("%s", tok->text);
    }

    printf("]");
}

static void print_ast_node(const AST_node_t *node, bool *has_sibling, size_t depth)
{
    if (!node)
    {
        print_indent(has_sibling, depth);
        printf("NULL\n");
        return;
    }

    print_indent(has_sibling, depth);

    printf("%s", AST_node_type_to_string(node->type));
    print_tokens(node);

    printf("\n");

    size_t child_count = array_size(node->children);

    for (size_t i = 0; i < child_count; ++i)
    {
        has_sibling[depth] = i + 1 < child_count;
        print_ast_node(node->children[i], has_sibling, depth + 1);
    }
}

void print_AST(const AST_node_t *root)
{
    if (!root)
    {
        printf("(null AST)\n");
        return;
    }

    bool has_sibling[256] = {0};
    print_ast_node(root, has_sibling, 0);
}

static const char *symbol_kind_to_string(symbol_kind_t kind)
{
    switch (kind)
    {
    case SYMBOL_VARIABLE:
        return "VARIABLE";
    case SYMBOL_FUNCTION:
        return "FUNCTION";
    case SYMBOL_PARAMETER:
        return "PARAMETER";
    default:
        return "UNKNOWN";
    }
}

static void print_type(const type_t *type)
{
    if (!type)
    {
        printf("<null>");
        return;
    }
    switch (type->kind)
    {
    case TYPE_VOID:
        printf("void");
        break;
    case TYPE_INT:
        printf("int");
        break;
    case TYPE_INTLIT:
        printf("intlit");
        break;
    case TYPE_POINTER:
        print_type(type->pointer.dest);
        printf("*");
        break;
    case TYPE_ARRAY:
        print_type(type->array.element);
        printf("[%zu]", type->array.size);
        break;
    case TYPE_FUNCTION:
    {
        printf("(");
        size_t count = array_size(type->function.parameter_types);
        for (size_t i = 0; i < count; ++i)
        {
            if (i != 0)
                printf(", ");
            print_type(type->function.parameter_types[i]);
        }
        printf(") -> ");
        print_type(type->function.return_type);
        break;
    }
    default:
        printf("<unknown type>");
        break;
    }
}

static void print_symbol_value(const symbol_t *symbol)
{
    if (!symbol->value.is_valid)
        return;

    printf(" = %" PRId64, symbol->value.val);
}

static void print_scope(const scope_t *scope, bool *has_sibling, size_t depth)
{
    if (!scope)
    {
        print_indent(has_sibling, depth);
        printf("NULL SCOPE\n");
        return;
    }
    print_indent(has_sibling, depth);
    printf("SCOPE\n");
    size_t symbol_count = array_size(scope->symbols);
    size_t child_count = array_size(scope->children);
    for (size_t i = 0; i < symbol_count; ++i)
    {
        const symbol_t *symbol = &scope->symbols[i];
        bool sibling = (i + 1 < symbol_count) || (i + 1 == symbol_count && child_count > 0);
        has_sibling[depth] = sibling;
        print_indent(has_sibling, depth);
        printf("%s %s : ", symbol_kind_to_string(symbol->kind), symbol->name);
        print_type(symbol->type);
        print_symbol_value(symbol);
        printf("\n");
    }
    for (size_t i = 0; i < child_count; ++i)
    {
        has_sibling[depth] = i + 1 < child_count;
        print_scope(scope->children[i], has_sibling, depth + 1);
    }
}

void print_scope_tree(const scope_t *root)
{
    if (!root)
    {
        printf("(null scope)\n");
        return;
    }
    bool has_sibling[256] = {0};
    print_scope(root, has_sibling, 0);
}

void print_hlir_type(ir_type_t *type)
{
    switch (type->kind)
    {
    case TYPE_INT:
        printf("i%d", type->integer.bits);
        break;

    case TYPE_POINTER:
        printf("ptr<");
        print_hlir_type(type->pointer.dest);
        printf(">");
        break;

    default:
        ASSERT(0);
    }
}

void print_hlir_value(ir_value_t *value)
{
    switch (value->kind)
    {
    case IR_VALUE_CONSTANT:
    {
        ir_constant_t *constant = (ir_constant_t *)value;
        printf("$%ld", constant->value);
        break;
    }

    default:
        printf("%%%ld: " ANSI_GRAY, value->id);
        print_hlir_type(value->type);
        printf(ANSI_RESET);
        break;
    }
}

void print_hlir_parameters(array_t(ir_parameter_t *) parameters)
{
    for (size_t i = 0; i < array_size(parameters); i++)
    {
        if (i > 0)
            printf(", ");

        print_hlir_value(&parameters[i]->value);
    }
}

void print_hlir_instruction(ir_inst_t *instruction)
{
    switch (instruction->opcode)
    {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_REM:
    case IR_CMP_EQ:
    case IR_CMP_NE:
    case IR_CMP_LT:
    case IR_CMP_LE:
    case IR_CMP_GT:
    case IR_CMP_GE:
    {
        print_hlir_value(&instruction->value);
        printf(" = " ANSI_CYAN);
        switch (instruction->opcode)
        {
        case IR_ADD:
            printf("add");
            break;
        case IR_SUB:
            printf("sub");
            break;
        case IR_MUL:
            printf("mul");
            break;
        case IR_DIV:
            printf("div");
            break;
        case IR_REM:
            printf("rem");
            break;
        case IR_CMP_EQ:
            printf("cmp eq");
            break;
        case IR_CMP_NE:
            printf("cmp ne");
            break;
        case IR_CMP_LT:
            printf("cmp lt");
            break;
        case IR_CMP_LE:
            printf("cmp le");
            break;
        case IR_CMP_GT:
            printf("cmp gt");
            break;
        case IR_CMP_GE:
            printf("cmp ge");
            break;
        default:
            ASSERT(0);
        }
        printf(ANSI_RESET " ");
        print_hlir_value(instruction->binary.lhs);
        printf(", ");
        print_hlir_value(instruction->binary.rhs);
        printf("\n");
        break;
    }

    case IR_LOAD:
    {
        print_hlir_value(&instruction->value);
        printf(" = " ANSI_CYAN "load " ANSI_RESET);
        print_hlir_value(instruction->load.address);
        printf("\n");
        break;
    }

    case IR_STORE:
    {
        printf(ANSI_CYAN "store " ANSI_RESET);
        print_hlir_value(instruction->store.address);
        printf(", ");
        print_hlir_value(instruction->store.value);
        printf("\n");
        break;
    }

    case IR_ALLOCA:
    {
        print_hlir_value(&instruction->value);
        printf(" = alloca ");
        ASSERT(instruction->value.type->kind == IR_TYPE_POINTER);
        print_hlir_type(instruction->value.type->pointer.dest);
        printf("\n");
        break;
    }

    case IR_RET:
    {
        printf(ANSI_CYAN "ret " ANSI_RESET);
        if (instruction->ret.value != NULL)
            print_hlir_value(instruction->ret.value);
        printf("\n");
        break;
    }

    case IR_COND_BR:
    {
        printf(ANSI_CYAN "cond_br " ANSI_RESET);
        print_hlir_value(instruction->cond_br.condition);
        printf(", %ld: " ANSI_GRAY "label" ANSI_RESET ", %ld: " ANSI_GRAY "label" ANSI_RESET "\n", instruction->cond_br.true_block->id, instruction->cond_br.false_block->id);
        break;
    }

    case IR_BR:
    {
        printf(ANSI_CYAN "br" ANSI_RESET " %ld: " ANSI_GRAY "label" ANSI_RESET "\n", instruction->br.target->id);
        break;
    }

    case IR_CALL:
    {
        print_hlir_value(&instruction->value);
        printf(" = " ANSI_CYAN "call " ANSI_RESET);
        printf("%s(", ((ir_function_t *)instruction->call.callee)->name);
        for (size_t i = 0; i < array_size(instruction->call.arguments); i++)
        {
            if (i > 0)
                printf(", ");
            print_hlir_value(instruction->call.arguments[i]);
        }
        printf(")\n");
        break;
    }

    default:
        ASSERT(0);
    }
}

void print_hlir_block(ir_block_t *block)
{
    printf("%ld:\n", block->id);
    for (size_t i = 0; i < array_size(block->instructions); i++)
    {
        print_simple_indent(4);
        print_hlir_instruction(block->instructions[i]);
    }

    if (block->terminator)
    {
        print_simple_indent(4);
        print_hlir_instruction(block->terminator);
    }
    else
    {
        log_warn("block without terminator");
    }
}

void print_hlir(ir_module_t *module)
{
    ir_global_t *global;
    foreach (module->globals, global)
    {
        print_hlir_type(global->value.type);
        printf(" %%%ld", global->value.id);
        switch (global->value.type->kind)
        {
        case IR_TYPE_INTEGER:
            printf(" = %ld", global->initial_value);
            break;

        default:
            break;
        }

        printf("\n");
    }

    if (array_size(module->globals) > 0)
        printf("\n");

    ir_function_t *func;
    foreach (module->functions, func)
    {
        printf("define ");
        print_hlir_type(func->function_type->function.return_type);
        printf(" @%s(", func->name);
        print_hlir_parameters(func->parameters);
        printf(") {\n");

        for (size_t i = 0; i < array_size(func->blocks); i++)
        {
            print_hlir_block(func->blocks[i]);
        }

        printf("}\n");
    }
}
