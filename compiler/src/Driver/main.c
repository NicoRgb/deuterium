#include <stdio.h>

#include "arguments.h"
#include "printer.h"
#include "parser.h"

int config_initialized = 0;
compiler_config_t g_config;

compiler_config_t *get_global_config(void)
{
    if (!config_initialized)
        return NULL;

    return &g_config;
}

int main(int argc, char *argv[])
{
    parse_arguments(argc, argv, &g_config);
    config_initialized = 1;

    init_lexer();
    tokstream_init("int main(void) {}");

    AST_node_t *AST = parse();
    print_AST(AST);

    return 0;
}
