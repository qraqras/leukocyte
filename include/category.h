#ifndef LEUKOCYTE_INCLUDE_CATEGORY_H
#define LEUKOCYTE_INCLUDE_CATEGORY_H

#include <stdbool.h>

/**
 * @brief Rule categories.
 */
typedef enum category_e
{
    CATEGORY_LAYOUT,
    CATEGORY_LINT,
} category_t;

bool leuko_rule_category_from_string(const char *str, category_t *out);
bool leuko_rule_category_to_string(category_t category, const char **out);

#endif /* LEUKOCYTE_INCLUDE_CATEGORY_H */
