#ifndef LEUKOCYTE_CONFIGS_RULES_LIST_H
#define LEUKOCYTE_CONFIGS_RULES_LIST_H

#include "common/category.h"
#include "common/registry/registry.h"
#include "configs/layout/indentation_consistency.h"
#include "rules/layout/indentation_consistency.h"

#define LEUKO_RULES_LIST \
    X(layout_indentation_consistency, LEUKO_RULE_CATEGORY_NAME_LAYOUT, LEUKO_RULE_NAME_INDENTATION_CONSISTENCY, &layout_indentation_consistency_rule, &layout_indentation_consistency_config_ops)

#endif /* LEUKOCYTE_CONFIGS_RULES_LIST_H */
