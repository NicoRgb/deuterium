#pragma once

#include "AST.h"
#include "scope.h"

void print_AST(const AST_node_t *root);
void print_scope_tree(const scope_t *root);
