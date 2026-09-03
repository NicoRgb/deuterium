#include <stdio.h>

#include "arguments.h"
#include "error.h"
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
    int result = 1;

    log_info("compiling file '%s'", filepath);

    ASSERT(filepath);
    char *content = read_file(filepath);
    ASSERT(content);

    init_errors(filepath, content);

    init_lexer();
    tokstream_init(content);

    AST_node_t *AST = parse();
    print_AST(AST);

    if (has_errors())
    {
        result = 0;
        emit_errors();
    }

    free(content);

    return result;
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
