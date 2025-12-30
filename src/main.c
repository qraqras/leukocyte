#include "cli/exit_code.h"
#include "cli/parser.h"
#include "cli/init.h"
#include "cli/sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/**
 * @brief Entry point for CLI application.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char *argv[])
{
    leuko_cli_options_t cli_opts = {0};
    leuko_parse_result_t parse_result = leuko_cli_options_parse(argc, argv, &cli_opts);
    switch (parse_result)
    {
    case LEUKO_CLI_OPTIONS_PARSE_OK:
        break;
    case LEUKO_CLI_OPTIONS_PARSE_PRINTED:
        return LEUKO_EXIT_OK;
    case LEUKO_CLI_OPTIONS_PARSE_ERROR:
        return LEUKO_EXIT_INVALID;
    default:
        return LEUKO_EXIT_INVALID;
    }

    /* Handle init subcommand */
    if (cli_opts.init)
    {
        /* Initialize .leukocyte templates (README, gitignore.template, configs/) */
        int rc = leuko_cli_init(NULL, false);
        leuko_cli_options_free(&cli_opts);
        return rc;
    }

    /* Handle sync subcommand */
    if (cli_opts.sync)
    {
        /* Use dedicated sync command implementation */
        int rc = leuko_cli_sync(NULL, NULL, NULL, NULL);
        leuko_cli_options_free(&cli_opts);
        return rc;
    }

    /* */

    leuko_cli_options_free(&cli_opts);
    return LEUKO_EXIT_OK;
}
