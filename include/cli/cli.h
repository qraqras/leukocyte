// CLI header
#ifndef LEUKOCYTE_CLI_H
#define LEUKOCYTE_CLI_H

#include <stddef.h>
#include <stdbool.h>

#include "formatter.h"

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

#endif // LEUKOCYTE_CLI_H
