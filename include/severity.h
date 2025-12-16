#ifndef LEUKOCYTE_CONFIGS_SEVERITY_H
#define LEUKOCYTE_CONFIGS_SEVERITY_H

#include <stdbool.h>

/* clang-format off */
#define SEVERITY_NAME_INFO       "info"
#define SEVERITY_NAME_REFACTOR   "refactor"
#define SEVERITY_NAME_CONVENTION "convention"
#define SEVERITY_NAME_WARNING    "warning"
#define SEVERITY_NAME_ERROR      "error"
#define SEVERITY_NAME_FATAL      "fatal"
/* clang-format on */

/// @brief Severity levels for diagnostics.
typedef enum severity_level_e
{
    SEVERITY_INFO,
    SEVERITY_REFACTOR,
    SEVERITY_CONVENTION,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    SEVERITY_FATAL,
} severity_level_t;

bool severity_level_from_string(const char *s, severity_level_t *out);

#endif /* LEUKOCYTE_CONFIGS_SEVERITY_H */
