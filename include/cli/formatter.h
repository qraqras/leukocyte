#ifndef INCLUDE_CLI_FORMATTER_H
#define INCLUDE_CLI_FORMATTER_H

#include <stdbool.h>
#include <stdint.h>

/* CLI output formatter enum. */
typedef enum
{
    CLI_FORMATTER_PROGRESS,
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

bool cli_formatter_from_string(const char *str, cli_formatter_t *out);

/* Print a single diagnostic using the selected formatter. 'file' may be NULL.
 * The message string must be owned by caller. The function returns true on
 * success (printed) or false if formatter suppressed output (e.g., QUIET).
 */
bool cli_formatter_print_diagnostic(cli_formatter_t fmt, const char *file, const char *message, int diag_id, uint8_t level);

#endif /* INCLUDE_CLI_FORMATTER_H */
