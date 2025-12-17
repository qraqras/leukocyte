#include "severity.h"
#include <string.h>

/**
 * @brief Convert a string to a severity_level_t enum value.
 * @param s Input string representing the severity level
 * @param out Pointer to output severity_level_t variable
 * @return true if conversion was successful, false otherwise
 */
bool severity_level_from_string(const char *s, severity_level_t *out)
{
    if (strcmp(s, SEVERITY_NAME_INFO) == 0)
    {
        *out = SEVERITY_INFO;
        return true;
    }
    if (strcmp(s, SEVERITY_NAME_REFACTOR) == 0)
    {
        *out = SEVERITY_REFACTOR;
        return true;
    }
    if (strcmp(s, SEVERITY_NAME_CONVENTION) == 0)
    {
        *out = SEVERITY_CONVENTION;
        return true;
    }
    if (strcmp(s, SEVERITY_NAME_WARNING) == 0)
    {
        *out = SEVERITY_WARNING;
        return true;
    }
    if (strcmp(s, SEVERITY_NAME_ERROR) == 0)
    {
        *out = SEVERITY_ERROR;
        return true;
    }
    if (strcmp(s, SEVERITY_NAME_FATAL) == 0)
    {
        *out = SEVERITY_FATAL;
        return true;
    }
    *out = SEVERITY_CONVENTION;
    return false;
}
