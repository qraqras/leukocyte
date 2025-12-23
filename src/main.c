#include "cli/exit_code.h"
#include "cli/parser.h"

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

    leuko_cli_options_free(&cli_opts);
    return LEUKO_EXIT_OK;
}
