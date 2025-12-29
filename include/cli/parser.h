#ifndef LEUKOCYTE_CLI_H
#define LEUKOCYTE_CLI_H

#include <stddef.h>
#include <stdbool.h>
#include "cli/formatter.h"

/**
 * @brief Result of parsing CLI options.
 */
typedef enum leuko_parse_result_e
{
    LEUKO_CLI_OPTIONS_PARSE_OK,      /* parse ok, continue execution */
    LEUKO_CLI_OPTIONS_PARSE_PRINTED, /* help or version printed, caller should exit with success */
    LEUKO_CLI_OPTIONS_PARSE_ERROR,   /* invalid options or parse error */
} leuko_parse_result_t;

/**
 * @brief Quick fix mode enum.
 */
typedef enum leuko_quick_fix_mode_e
{
    LEUKO_QUICK_FIX_MODE_NONE,   /* fix none */
    LEUKO_QUICK_FIX_MODE_SAFE,   /* fix safe */
    LEUKO_QUICK_FIX_MODE_UNSAFE, /* fix all */
} leuko_quick_fix_mode_t;

/**
 * @brief Command line options structure.
 */
typedef struct leuko_cli_options_s
{
    char **paths;                          /* paths to analyze */
    size_t paths_count;                    /* number of paths */
    char *config_path;                     /* path to configuration file */
    leuko_cli_formatter_t formatter;       /* formatter to use */
    char **only;                           /* only rules to apply */
    size_t only_count;                     /* number of only rules */
    char **except;                         /* rules to exclude */
    size_t except_count;                   /* number of except rules */
    leuko_quick_fix_mode_t quick_fix_mode; /* quick fix mode */
    bool parallel;                         /* run analysis in parallel */
    bool sync;                             /* sync config */
} leuko_cli_options_t;

leuko_parse_result_t leuko_cli_options_parse(int argc, char *argv[], leuko_cli_options_t *opts);
void leuko_cli_options_free(leuko_cli_options_t *opts);

#endif /* LEUKOCYTE_CLI_H */
