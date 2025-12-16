#ifndef LEUKOCYTE_CONFIGS_SEVERITY_H
#define LEUKOCYTE_CONFIGS_SEVERITY_H

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

#endif /* LEUKOCYTE_CONFIGS_SEVERITY_H */
