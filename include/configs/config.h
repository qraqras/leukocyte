#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stddef.h>

#include "configs/rule_config.h"
#include "rules_list.h"

/**
 * @brief Main configuration structure containing all rule configurations.
 * @note `all_include` and `all_exclude` are for global AllCops-level patterns.
 */
typedef struct config_s
{
#define X(field, cat_name, sname, rule_ptr, ops_ptr) leuko_rule_config_t *field;
    LEUKO_RULES_LIST
#undef X
    char **all_include;
    size_t all_include_count;
    char **all_exclude;
    size_t all_exclude_count;
} config_t;

void initialize_config(config_t *cfg);
void free_config(config_t *cfg);
leuko_rule_config_t *get_rule_config_by_index(config_t *cfg, size_t idx);
size_t config_count(void);

#endif /* LEUKOCYTE_CONFIGS_CONFIG_H */
