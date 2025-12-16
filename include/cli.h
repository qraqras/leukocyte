// CLI header
#ifndef LEUKOCYTE_CLI_H
#define LEUKOCYTE_CLI_H

#include <stddef.h>
#include <stdbool.h>

/// @brief CLI output format enum.
typedef enum
{
    CLI_FORMATTER_PROGRESS, // default
    CLI_FORMATTER_AUTO_GEN,
    CLI_FORMATTER_CLANG_STYLE,
    CLI_FORMATTER_FUUBAR_STYLE,
    CLI_FORMATTER_PACMAN_STYLE,
    CLI_FORMATTER_EMACS_STYLE,
    CLI_FORMATTER_SIMPLE,
    CLI_FORMATTER_QUIET,
    CLI_FORMATTER_FILE_LIST,
    CLI_FORMATTER_JSON,
    CLI_FORMATTER_JUNIT_STYLE,
    CLI_FORMATTER_OFFENCE_COUNT,
    CLI_FORMATTER_WORST_OFFENDERS,
    CLI_FORMATTER_HTML,
    CLI_FORMATTER_MARKDOWN,
    CLI_FORMATTER_TAP,
    CLI_FORMATTER_GITHUB_ACTIONS,
} cli_formatter_t;

typedef enum
{
    QUICK_FIX_MODE_OFF,
    QUICK_FIX_MODE_SAFE,
    QUICK_FIX_MODE_UNSAFE,
} quick_fix_mode_t;

/// @brief CLI options structure.
typedef struct
{
    char **paths;
    size_t paths_count;
    char *config_path;
    cli_formatter_t formatter;
    char **only;
    size_t only_count;
    char **except;
    size_t except_count;
    quick_fix_mode_t quick_fix_mode;
} cli_options_t;

int parse_command_line(int argc, char *argv[], cli_options_t *opts);
void cli_options_free(cli_options_t *opts);

#endif /* LEUKOCYTE_CLI_H */
