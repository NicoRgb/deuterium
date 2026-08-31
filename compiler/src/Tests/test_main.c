#include <testing.h>

#include "arguments.h"

compiler_config_t g_config;

compiler_config_t *get_global_config(void)
{
    g_config.mode = COMPILER_MODE_COMPILE;
    g_config.verbosity = COMPILER_VERBOSITY_ALL;
    g_config.outfile = NULL;
    g_config.infiles = NULL_ARRAY;

    return &g_config;
}

int main(void)
{
    run_tests();
    free_tests();
}
