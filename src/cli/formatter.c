#include <string.h>
#include <stdio.h>
#include "cli/formatter.h"

/**
 * @brief Mapping of formatter strings to enum values.
 */
static const struct
{
    const char *str;
    const leuko_cli_formatter_t formatter;
} map[] = {
    /* clang-format off */
    {LEUKO_CLI_FORMATTER_NAME_PROGRESS       , LEUKO_CLI_FORMATTER_PROGRESS       },
    {LEUKO_CLI_FORMATTER_NAME_AUTO_GEN       , LEUKO_CLI_FORMATTER_AUTO_GEN       },
    {LEUKO_CLI_FORMATTER_NAME_CLANG_STYLE    , LEUKO_CLI_FORMATTER_CLANG_STYLE    },
    {LEUKO_CLI_FORMATTER_NAME_FUUBAR_STYLE   , LEUKO_CLI_FORMATTER_FUUBAR_STYLE   },
    {LEUKO_CLI_FORMATTER_NAME_PACMAN_STYLE   , LEUKO_CLI_FORMATTER_PACMAN_STYLE   },
    {LEUKO_CLI_FORMATTER_NAME_EMACS_STYLE    , LEUKO_CLI_FORMATTER_EMACS_STYLE    },
    {LEUKO_CLI_FORMATTER_NAME_SIMPLE         , LEUKO_CLI_FORMATTER_SIMPLE         },
    {LEUKO_CLI_FORMATTER_NAME_QUIET          , LEUKO_CLI_FORMATTER_QUIET          },
    {LEUKO_CLI_FORMATTER_NAME_FILE_LIST      , LEUKO_CLI_FORMATTER_FILE_LIST      },
    {LEUKO_CLI_FORMATTER_NAME_JSON           , LEUKO_CLI_FORMATTER_JSON           },
    {LEUKO_CLI_FORMATTER_NAME_JUNIT_STYLE    , LEUKO_CLI_FORMATTER_JUNIT_STYLE    },
    {LEUKO_CLI_FORMATTER_NAME_OFFENCE_COUNT  , LEUKO_CLI_FORMATTER_OFFENCE_COUNT  },
    {LEUKO_CLI_FORMATTER_NAME_WORST_OFFENDERS, LEUKO_CLI_FORMATTER_WORST_OFFENDERS},
    {LEUKO_CLI_FORMATTER_NAME_HTML           , LEUKO_CLI_FORMATTER_HTML           },
    {LEUKO_CLI_FORMATTER_NAME_MARKDOWN       , LEUKO_CLI_FORMATTER_MARKDOWN       },
    {LEUKO_CLI_FORMATTER_NAME_TAP            , LEUKO_CLI_FORMATTER_TAP            },
    {LEUKO_CLI_FORMATTER_NAME_GITHUB_ACTIONS , LEUKO_CLI_FORMATTER_GITHUB_ACTIONS },
    /* clang-format on */
};

/**
 * @brief Convert string to leuko_cli_formatter_t enum value.
 * @param str Input string
 * @param out Output parameter to store the corresponding leuko_cli_formatter_t value
 * @return true if conversion was successful, false otherwise
 */
bool leuko_cli_formatter_from_string(const char *str, leuko_cli_formatter_t *out)
{
    *out = LEUKO_CLI_FORMATTER_PROGRESS;
    if (!str)
    {
        return false;
    }
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
    {
        if (strcmp(str, map[i].str) == 0)
        {
            *out = map[i].formatter;
            return true;
        }
    }
    return false;
}
