#ifndef LEUKOCYTE_CONFIGS_SEVERITY_H
#define LEUKOCYTE_CONFIGS_SEVERITY_H

#include <stdbool.h>

/* clang-format off */
#define LEUKO_SEVERITY_NAME_INFO       "info"
#define LEUKO_SEVERITY_NAME_REFACTOR   "refactor"
#define LEUKO_SEVERITY_NAME_CONVENTION "convention"
#define LEUKO_SEVERITY_NAME_WARNING    "warning"
#define LEUKO_SEVERITY_NAME_ERROR      "error"
#define LEUKO_SEVERITY_NAME_FATAL      "fatal"
/* clang-format on */

/**
 * @brief Severity level enumeration.
 */
typedef enum leuko_severity_e
{
    LEUKO_SEVERITY_INFO,
    LEUKO_SEVERITY_REFACTOR,
    LEUKO_SEVERITY_CONVENTION,
    LEUKO_SEVERITY_WARNING,
    LEUKO_SEVERITY_ERROR,
    LEUKO_SEVERITY_FATAL,
} leuko_severity_t;

bool leuko_severity_level_from_string(const char *s, leuko_severity_t *out);

#endif /* LEUKOCYTE_CONFIGS_SEVERITY_H */
