#include <stdio.h>
#include <arguments.h>

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

    return 0;
}
