#ifndef LEUKOCYTE_CONFIGS_RULES_LIST_H
#define LEUKOCYTE_CONFIGS_RULES_LIST_H

#include "common/category.h"
#include "common/registry/registry.h"
#include "configs/layout/indentation_consistency.h"
#include "rules/layout/indentation_consistency.h"

/* Per-category rule lists (use X-macro entries for each rule) */
#define LEUKO_RULES_LAYOUT \
    X(layout_indentation_consistency, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_CONSISTENCY, &layout_indentation_consistency_rule, &layout_indentation_consistency_config_ops)

/* Top-level categories macro: pairs of (category_name, macro) expanded by generated_rules.c */
#define LEUKO_RULES_CATEGORIES \
    CATEGORY(LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULES_LAYOUT)

/* Backwards-compatible flat list (concatenate category macros) */
#define LEUKO_RULES_LIST \
    LEUKO_RULES_LAYOUT

#endif /* LEUKOCYTE_CONFIGS_RULES_LIST_H */
