#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include "cli/parser.h"
#include "cli/formatter.h"
#include "common/registry.h"
#include "version.h"

/**
 * @brief  Print help message to stdout.
 */
static void print_help(void)
{
    printf("Usage: leuko [options] [file1, file2, ...]\n");
    printf("Options:\n");
    printf("  -a, --auto-correct          Automatically fix safe issues\n");
    printf("  -A, --auto-correct-all      Automatically fix all issues (including unsafe)\n");
    printf("  -c, --config <path>         Specify configuration file path\n");
    printf("      --except <rule1,rule2>  Exclude specific rules\n");
    printf("  -x, --fix-layout            Fix layout issues (safe only)\n");
    printf("  -f, --format <format>       Specify output format (text, json)\n");
    printf("  -h, --help                  Show this help message\n");
    printf("  -v, --version               Show version information\n");
    printf("      --only <rule1,rule2>    Only include specific rules\n");
    printf("      --parallel              Enable automatic parallel execution (set jobs to CPU count)\n");
    printf("      --sync                  Regenerate .leukocyte.resolved.json by searching for .rubocop.yml in the current directory or its parents\n");
}

/**
 * @brief  Print version information to stdout.
 */
static void print_version(void)
{
    printf(LEUKO_VERSION "\n");
}

/**
 * @brief Initialize cli_options_t structure with default values.
 * @param cli_opts Pointer to cli_options_t struct to initialize
 */
static bool leuko_cli_options_initialize(leuko_cli_options_t *cli_opts)
{
    if (!cli_opts)
    {
        return false;
    }
    memset(cli_opts, 0, sizeof(*cli_opts));
    cli_opts->paths = NULL;
    cli_opts->paths_count = 0;
    cli_opts->config_path = NULL;
    cli_opts->formatter = LEUKO_CLI_FORMATTER_PROGRESS;
    cli_opts->only = NULL;
    cli_opts->only_count = 0;
    cli_opts->except = NULL;
    cli_opts->except_count = 0;
    cli_opts->quick_fix_mode = LEUKO_QUICK_FIX_MODE_NONE;
    cli_opts->parallel = false;
    return true;
}

/**
 * @brief Parse command line arguments into cli_options_t.
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @param opts Pointer to cli_options_t struct to populate
 * @return 0 on success, 1 if help/version printed, -1 on error
 */
leuko_parse_result_t leuko_cli_options_parse(int argc, char *argv[], leuko_cli_options_t *cli_opts)
{
    if (!leuko_cli_options_initialize(cli_opts))
    {
        return LEUKO_CLI_OPTIONS_PARSE_ERROR;
    }

    static struct option long_options[] = {
        /* clang-format off */
        {"auto-correct"    , no_argument      , 0, 'a'},
        {"auto-correct-all", no_argument      , 0, 'A'},
        {"config"          , required_argument, 0, 'c'},
        {"except"          , required_argument, 0, 0  },
        {"fix-layout"      , no_argument      , 0, 'x'},
        {"format"          , required_argument, 0, 'f'},
        {"help"            , no_argument      , 0, 'h'},
        {"only"            , required_argument, 0, 0  },
        {"version"         , no_argument      , 0, 'v'},
        {"parallel"        , no_argument      , 0, 0  },
        {"sync"            , no_argument      , 0, 0  },
        {0, 0, 0, 0}
        /* clang-format on */
    };

    for (;;)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "aAc:xf:hv", long_options, &option_index);
        if (c == -1)
        {
            break;
        }
        switch (c)
        {
        case 'a':
            cli_opts->quick_fix_mode = LEUKO_QUICK_FIX_MODE_SAFE;
            break;
        case 'A':
            cli_opts->quick_fix_mode = LEUKO_QUICK_FIX_MODE_UNSAFE;
            break;
        case 'x':
            /* Treat `-x` as equivalent to `-a --only Layout` */
            cli_opts->quick_fix_mode = LEUKO_QUICK_FIX_MODE_SAFE;
            if (!leuko_str_arr_push(&cli_opts->only, &cli_opts->only_count, LEUKO_RULE_CATEGORY_NAME_LAYOUT))
            {
                return LEUKO_CLI_OPTIONS_PARSE_ERROR;
            }
            break;
        case 'h':
            print_help();
            return LEUKO_CLI_OPTIONS_PARSE_PRINTED;
        case 'v':
            print_version();
            return LEUKO_CLI_OPTIONS_PARSE_PRINTED;
        case 'f':
            leuko_cli_formatter_from_string(optarg, &cli_opts->formatter);
            break;
        case 'c':
            cli_opts->config_path = strdup(optarg);
            break;
        case 0:
            if (strcmp(long_options[option_index].name, "except") == 0)
            {
                size_t tmp_count = 0;
                char **tmp = NULL;
                if (!leuko_str_arr_split(optarg, ",", &tmp, &tmp_count))
                {
                    return LEUKO_CLI_OPTIONS_PARSE_ERROR;
                }
                if (tmp != NULL && tmp_count > 0)
                {
                    if (!leuko_str_arr_concat(&cli_opts->except, &cli_opts->except_count, tmp, tmp_count))
                    {
                        for (size_t i = 0; i < tmp_count; ++i)
                        {
                            free(tmp[i]);
                        }
                        free(tmp);
                        return LEUKO_CLI_OPTIONS_PARSE_ERROR;
                    }
                    free(tmp);
                }
            }
            if (strcmp(long_options[option_index].name, "only") == 0)
            {
                size_t tmp_count = 0;
                char **tmp = NULL;
                if (!leuko_str_arr_split(optarg, ",", &tmp, &tmp_count))
                {
                    return LEUKO_CLI_OPTIONS_PARSE_ERROR;
                }
                if (tmp != NULL && tmp_count > 0)
                {
                    if (!leuko_str_arr_concat(&cli_opts->only, &cli_opts->only_count, tmp, tmp_count))
                    {
                        for (size_t i = 0; i < tmp_count; ++i)
                        {
                            free(tmp[i]);
                        }
                        free(tmp);
                        return LEUKO_CLI_OPTIONS_PARSE_ERROR;
                    }
                    free(tmp);
                }
            }
            if (strcmp(long_options[option_index].name, "parallel") == 0)
            {
                cli_opts->parallel = true;
            }
            if (strcmp(long_options[option_index].name, "sync") == 0)
            {
                cli_opts->sync = true;
            }
            break;
        case '?':
            return LEUKO_CLI_OPTIONS_PARSE_ERROR;
        default:
            break;
        }
    }

    // Remaining args are paths, if any.
    if (optind < argc)
    {
        cli_opts->paths_count = argc - optind;
        cli_opts->paths = calloc(cli_opts->paths_count, sizeof(char *));
        for (int i = 0; optind < argc; ++i, ++optind)
        {
            cli_opts->paths[i] = strdup(argv[optind]);
        }
    }
    return 0;
}

/**
 * @brief Free memory allocated in cli_options_t.
 * @param opts Pointer to cli_options_t struct to free
 */
void leuko_cli_options_free(leuko_cli_options_t *opts)
{
    if (!opts)
    {
        return;
    }
    for (size_t i = 0; i < opts->paths_count; ++i)
    {
        free(opts->paths[i]);
    }
    free(opts->paths);
    for (size_t i = 0; i < opts->only_count; ++i)
    {
        free(opts->only[i]);
    }
    free(opts->only);
    for (size_t i = 0; i < opts->except_count; ++i)
    {
        free(opts->except[i]);
    }
    free(opts->except);
    free(opts->config_path);
}
