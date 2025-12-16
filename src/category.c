#include <string.h>

#include "category.h"

/// @brief Map of strings to category_t enum values.
static const struct
{
    const char *str;
    category_t rule_category;
} rule_category_map[] = {
    {"layout", CATEGORY_LAYOUT},
    {"Layout", CATEGORY_LAYOUT},
    {"lint", CATEGORY_LINT},
    {"Lint", CATEGORY_LINT},
};

/// @brief Convert string to rule_category_t enum value.
/// @param str Input string
/// @param out Output parameter to store the corresponding rule_category_t value
/// @return true if conversion was successful, false otherwise
bool rule_category_from_string(const char *str, category_t *out)
{
    if (!str)
        return false;
    for (size_t i = 0; i < sizeof(rule_category_map) / sizeof(rule_category_map[0]); ++i)
    {
        if (strcmp(str, rule_category_map[i].str) == 0)
        {
            *out = rule_category_map[i].rule_category;
            return true;
        }
    }
    return false;
}

/// @brief Convert category_t enum value to string.
/// @param category Input category_t value
/// @param out Output parameter to store the corresponding string representation
/// @return true if conversion was successful, false otherwise
bool rule_category_to_string(category_t category, const char **out)
{
    for (size_t i = 0; i < sizeof(rule_category_map) / sizeof(rule_category_map[0]); ++i)
    {
        if (category == rule_category_map[i].rule_category)
        {
            *out = rule_category_map[i].str;
            return true;
        }
    }
    return false;
}
