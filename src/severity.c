#include "severity.h"
#include <string.h>

/**
 * @brief Convert a string to a leuko_severity_t enum value.
 * @param s Input string representing the severity level
 * @param out Pointer to output leuko_severity_t variable
 * @return true if conversion was successful, false otherwise
 */
bool leuko_severity_level_from_string(const char *s, leuko_severity_t *out)
{
    if (strcmp(s, LEUKO_SEVERITY_NAME_INFO) == 0)
    {
        *out = LEUKO_SEVERITY_INFO;
        return true;
    }
    if (strcmp(s, LEUKO_SEVERITY_NAME_REFACTOR) == 0)
    {
        *out = LEUKO_SEVERITY_REFACTOR;
        return true;
    }
    if (strcmp(s, LEUKO_SEVERITY_NAME_CONVENTION) == 0)
    {
        *out = LEUKO_SEVERITY_CONVENTION;
        return true;
    }
    if (strcmp(s, LEUKO_SEVERITY_NAME_WARNING) == 0)
    {
        *out = LEUKO_SEVERITY_WARNING;
        return true;
    }
    if (strcmp(s, LEUKO_SEVERITY_NAME_ERROR) == 0)
    {
        *out = LEUKO_SEVERITY_ERROR;
        return true;
    }
    if (strcmp(s, LEUKO_SEVERITY_NAME_FATAL) == 0)
    {
        *out = LEUKO_SEVERITY_FATAL;
        return true;
    }
    *out = LEUKO_SEVERITY_CONVENTION;
    return false;
}
