#ifndef LEUKOCYTE_CONFIGS_RULES_LIST_H
#define LEUKOCYTE_CONFIGS_RULES_LIST_H

#include "common/rule_registry.h"
#include "common/rule_registry.h"
#include "configs/layout/indentation_consistency.h"
#include "rules/layout/indentation_consistency.h"
#include "configs/layout/indentation_width.h"
#include "rules/layout/indentation_width.h"
#include "configs/layout/indentation_style.h"
#include "rules/layout/indentation_style.h"
#include "configs/layout/line_length.h"
#include "rules/layout/line_length.h"

/**
 * @brief Macro listing all rules in the `Layout` category.
 */
#define LEUKO_RULES_LAYOUT                                                                                                                                                                        \
    X(layout_indentation_consistency, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_CONSISTENCY, &layout_indentation_consistency_rule, &layout_indentation_consistency_config_ops) \
    X(layout_indentation_width, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_WIDTH, &layout_indentation_width_rule, &layout_indentation_width_config_ops)                         \
    X(layout_indentation_style, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_STYLE, &layout_indentation_style_rule, &layout_indentation_style_config_ops)                         \
    X(layout_line_length, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_LINE_LENGTH, &layout_line_length_rule, &layout_line_length_config_ops)

/**
 * @brief Macro listing all rules in the `Lint` category.
 */
#define LEUKO_RULES_LINT /* Currently empty */

/**
 * @brief Macro listing all rule categories and their rules.
 */
#define LEUKO_RULES_CATEGORIES                                    \
    CATEGORY(LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULES_LAYOUT) \
    CATEGORY(LEUKO_RULE_CATEGORY_NAME_LINT, LEUKO_RULES_LINT)

/**
 * @brief Macro listing all rules across all categories.
 */
#define LEUKO_RULES_LIST \
    LEUKO_RULES_LAYOUT   \
    LEUKO_RULES_LINT

#endif /* LEUKOCYTE_CONFIGS_RULES_LIST_H */
