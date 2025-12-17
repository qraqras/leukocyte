#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "cli/formatter.h"

/**
 * @brief Map of string representations to cli_formatter_t enum values.
 */
static const struct
{
    const char *str;
    const cli_formatter_t formatter;
} formatter_map[] = {
    {"progress", CLI_FORMATTER_PROGRESS},
    {"autogenconf", CLI_FORMATTER_AUTO_GEN},
    {"clang", CLI_FORMATTER_CLANG_STYLE},
    {"fuubar", CLI_FORMATTER_FUUBAR_STYLE},
    {"pacman", CLI_FORMATTER_PACMAN_STYLE},
    {"emacs", CLI_FORMATTER_EMACS_STYLE},
    {"simple", CLI_FORMATTER_SIMPLE},
    {"quiet", CLI_FORMATTER_QUIET},
    {"files", CLI_FORMATTER_FILE_LIST},
    {"json", CLI_FORMATTER_JSON},
    {"junit", CLI_FORMATTER_JUNIT_STYLE},
    {"offenses", CLI_FORMATTER_OFFENCE_COUNT},
    {"worst", CLI_FORMATTER_WORST_OFFENDERS},
    {"html", CLI_FORMATTER_HTML},
    {"markdown", CLI_FORMATTER_MARKDOWN},
    {"tap", CLI_FORMATTER_TAP},
    {"github", CLI_FORMATTER_GITHUB_ACTIONS},
};

/**
 * @brief Convert string to cli_formatter_t enum value.
 * @param str Input string
 * @param out Output parameter to store the corresponding cli_formatter_t value
 * @return true if conversion was successful, false otherwise
 */
bool cli_formatter_from_string(const char *str, cli_formatter_t *out)
{
    *out = CLI_FORMATTER_PROGRESS;
    if (!str)
    {
        return false;
    }
    for (size_t i = 0; i < sizeof(formatter_map) / sizeof(formatter_map[0]); ++i)
    {
        if (strcmp(str, formatter_map[i].str) == 0)
        {
            *out = formatter_map[i].formatter;
            return true;
        }
    }
    return false;
}
