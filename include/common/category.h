#ifndef LEUKOCYTE_INCLUDE_CATEGORY_H
#define LEUKOCYTE_INCLUDE_CATEGORY_H

/* clang-format off */
#define LEUKO_RULE_CATEGORY_NAME_LAYOUT "Layout"
#define LEUKO_RULE_CATEGORY_NAME_LINT   "Lint"
/* clang-format on */

/**
 * @brief Rule categories.
 */
typedef enum leuko_relu_category_e
{
    LEUKO_RULE_CATEGORY_LAYOUT,
    LEUKO_RULE_CATEGORY_LINT,
} leuko_rule_category_t;

#endif /* LEUKOCYTE_INCLUDE_CATEGORY_H */
