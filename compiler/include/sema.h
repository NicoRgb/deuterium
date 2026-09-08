#pragma once

#include "scope.h"

void create_scopes(AST_node_t *AST);
symbol_const_value_t eval_constant_expression(AST_node_t *node);
