#ifndef LEUKOCYTE_CONFIGS_RULES_LIST_H
#define LEUKOCYTE_CONFIGS_RULES_LIST_H

#include "category.h"
#include "rule_registry.h"
#include "configs/layout.h"

#define LEUKO_RULES_LIST \
    X(layout_indentation_consistency, LEUKO_LAYOUT, LEUKO_SHORTNAME_INDENTATION_CONSISTENCY, &layout_indentation_consistency_rule, &layout_indentation_consistency_config_ops)

#endif /* LEUKOCYTE_CONFIGS_RULES_LIST_H */
