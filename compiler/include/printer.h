#pragma once

#include "AST.h"
#include "scope.h"
#include "hlir.h"

const char *AST_node_type_to_string(AST_node_type_t type);
void print_AST(const AST_node_t *root);
void print_scope_tree(const scope_t *root);
void print_hlir(ir_module_t *module);
