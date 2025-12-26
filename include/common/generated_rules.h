#ifndef LEUKOCYTE_COMMON_GENERATED_RULES_H
#define LEUKOCYTE_COMMON_GENERATED_RULES_H

#include <stddef.h>
#include "configs/common/rule_config.h"

/* Forward declare rule type as opaque to avoid heavy includes */
typedef struct rule_s rule_t;

/* Registry structures used by materialize */
typedef struct
{
    const char *name; /* rule name, e.g. "IndentationConsistency" */
    rule_t *rule;     /* pointer to rule instance (opaque) */
    const leuko_rule_config_handlers_t *handlers;
} leuko_rule_registry_entry_t;

typedef struct
{
    const char *category; /* "Layout" */
    const leuko_rule_registry_entry_t *entries;
    size_t count;
} leuko_rule_category_registry_t;

/* Accessors (implemented in generated_rules.c) */
const leuko_rule_category_registry_t *leuko_get_rule_categories(void);
size_t leuko_get_rule_category_count(void);

/* Find a rule's global index by category and name. Returns SIZE_MAX if not found */
size_t leuko_rule_find_index(const char *category, const char *name);

#endif /* LEUKOCYTE_COMMON_GENERATED_RULES_H */
