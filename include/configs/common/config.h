#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stddef.h>

#include "configs/common/rule_config.h"
#include "common/registry/list.h"

/**
 * @brief Main configuration structure containing all rule configurations.
 * @note `all_include` and `all_exclude` are for global AllCops-level patterns.
 */
typedef struct leuko_config_s
{
#define X(field, cat_name, sname, rule_ptr, ops_ptr) leuko_rule_config_t *field;
    LEUKO_RULES_LIST
#undef X
    char **all_include;
    size_t all_include_count;
    char **all_exclude;
    size_t all_exclude_count;
} leuko_config_t;

void leuko_config_initialize(leuko_config_t *cfg);
void leuko_config_free(leuko_config_t *cfg);
leuko_rule_config_t *leuko_rule_config_get_by_index(leuko_config_t *cfg, size_t idx);
size_t leuko_config_count(void);

#endif /* LEUKOCYTE_CONFIGS_CONFIG_H */
