#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stddef.h>
#include "common/registry/list.h"
#include "configs/common/rule_config.h"

#define LEUKO_INHERIT_FROM "inherit_from"
#define LEUKO_INHERIT_MODE "inherit_mode"
#define LEUKO_INHERIT_MODE_MERGE "merge"
#define LEUKO_INHERIT_MODE_OVERRIDE "override"

/**
 * @brief Main configuration structure containing all rule configurations.
 * @note `all_include` and `all_exclude` are for global AllCops-level patterns.
 */
typedef struct leuko_config_s
{
    /* Global AllCops include/exclude (used when rules_config absent) */
    char **all_include;
    size_t all_include_count;
    char **all_exclude;
    size_t all_exclude_count;

#define X(field, cat_name, sname, rule_ptr, ops_ptr) leuko_rule_config_t *field;
    LEUKO_RULES_LIST
#undef X
} leuko_config_t;

void leuko_config_initialize(leuko_config_t *cfg);
void leuko_config_free(leuko_config_t *cfg);
leuko_rule_config_t *leuko_rule_config_get_by_index(leuko_config_t *cfg, size_t idx);
size_t leuko_config_count(void);

#endif /* LEUKOCYTE_CONFIGS_CONFIG_H */
