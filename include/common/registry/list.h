#ifndef LEUKOCYTE_CONFIGS_RULES_LIST_H
#define LEUKOCYTE_CONFIGS_RULES_LIST_H

#include "common/category.h"
#include "common/generated_rules.h"
#include "configs/layout/indentation_consistency.h"
#include "rules/layout/indentation_consistency.h"
#include "configs/layout/indentation_width.h"
#include "rules/layout/indentation_width.h"
#include "configs/layout/indentation_style.h"
#include "rules/layout/indentation_style.h"
#include "configs/layout/line_length.h"
#include "rules/layout/line_length.h"

/* Per-category rule lists (use X-macro entries for each rule) */
#define LEUKO_RULES_LAYOUT \
    X(layout_indentation_consistency, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_CONSISTENCY, &layout_indentation_consistency_rule, &layout_indentation_consistency_config_ops) \
    X(layout_indentation_width, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_WIDTH, &layout_indentation_width_rule, &layout_indentation_width_config_ops) \
    X(layout_indentation_style, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_STYLE, &layout_indentation_style_rule, &layout_indentation_style_config_ops) \
    X(layout_line_length, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_LINE_LENGTH, &layout_line_length_rule, &layout_line_length_config_ops)

/* Lint Rules (none implemented yet) */
#define LEUKO_RULES_LINT

/* Top-level categories macro: pairs of (category_name, macro) expanded by generated_rules.c */
#define LEUKO_RULES_CATEGORIES                                    \
    CATEGORY(LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULES_LAYOUT) \
    CATEGORY(LEUKO_RULE_CATEGORY_NAME_LINT, LEUKO_RULES_LINT)

/* Backwards-compatible flat list (concatenate category macros) */
#define LEUKO_RULES_LIST \
    LEUKO_RULES_LAYOUT   \
    LEUKO_RULES_LINT

#endif /* LEUKOCYTE_CONFIGS_RULES_LIST_H */
