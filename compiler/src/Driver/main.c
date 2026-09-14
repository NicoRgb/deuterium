#include <stdio.h>

#include "arguments.h"
#include "error.h"
#include "printer.h"
#include "parser.h"
#include "sema.h"
#include "hlir.h"

int config_initialized = 0;
compiler_config_t g_config;

compiler_config_t *get_global_config(void)
{
    if (!config_initialized)
        return NULL;

    return &g_config;
}

static char *read_file(const char *filepath)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp)
    {
        log_error("failed to open file");
        exit(EXIT_FAILURE);
    }

    if (fseek(fp, 0, SEEK_END))
    {
        log_error("failed to fseek");
        exit(EXIT_FAILURE);
    }

    size_t size = ftell(fp);

    if (fseek(fp, 0, SEEK_SET))
    {
        log_error("failed to fseek");
        exit(EXIT_FAILURE);
    }

    char *content = malloc(size + 1);
    if (!content)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    content[size] = 0;
    fread(content, size, 1, fp);

    return content;
}

static int compile_unit(const char *filepath)
{
    log_info("compiling file '%s'", filepath);

    ASSERT(filepath);
    char *content = read_file(filepath);
    ASSERT(content);

    log_info("compilation unit:\n");
    printf("%s\n\n", content);

    init_errors(filepath, content);

    init_lexer();
    tokstream_init(content);

    AST_node_t *AST = parse();

    if (has_errors())
    {
        emit_errors();
        free(content);

        return 0;
    }
    printf("\n");
    log_info("AST:\n");
    print_AST(AST);

    create_scopes(AST);

    if (has_errors())
    {
        emit_errors();
        free_scopes();
        free(content);

        return 0;
    }
    log_info("typed AST:\n");
    print_AST(AST);
    printf("\n");

    desugar_typed_AST(AST);
    log_info("desugared AST:\n");
    print_AST(AST);
    printf("\n");

    scope_t *scope = get_global_scope();
    log_info("symbol tables:\n");
    print_scope_tree(scope);
    printf("\n");

    ir_module_t *module = generate_high_level_ir(scope);
    log_info("high level intermediate representation:\n");
    print_hlir(module);

    free_scopes();
    free(content);

    return 1;
}

int main(int argc, char *argv[])
{
    parse_arguments(argc, argv, &g_config);
    config_initialized = 1;

    int success = 1;
    for (size_t i = 0; i < array_size(g_config.infiles); i++)
    {
        if (!compile_unit(g_config.infiles[i]))
            success = 0;
    }

    if (!success)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
