#include <arguments.h>

#include <unistd.h>
#include <getopt.h>

#include "logger.h"

void parse_arguments(int argc, char *argv[], compiler_config_t *out_config)
{
    ASSERT(out_config);

    out_config->mode = COMPILER_MODE_COMPILE;
    out_config->verbosity = COMPILER_VERBOSITY_NORMAL;
    out_config->outfile = "a.out";
    out_config->infiles = array_create(char *);

    int c = 0;
    for (;;)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"verbose", no_argument, 0, 'v'},
            {"version", no_argument, 0, 'm'},
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "vo:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'v':
            if (out_config->verbosity != COMPILER_VERBOSITY_ALL)
            {
                log_info("verbose flag set");
                out_config->verbosity = COMPILER_VERBOSITY_ALL;
            }
            break;
        case 'm':
            log_info("version flag set");
            out_config->mode = COMPILER_MODE_VERSION;
            break;
        case 'o':
            ASSERT(optarg);
            log_info("output file: %s", optarg);
            out_config->outfile = optarg;
            break;
        default:
            log_error("corrupted getopt state. getopt_long returned %d", c);
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
    {
        while (optind < argc)
        {
            ASSERT(argv[optind]);
            log_info("input file: %s", argv[optind]);
            array_push(out_config->infiles, argv[optind++]);
        }
    }
}
