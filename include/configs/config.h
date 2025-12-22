#ifndef LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H
#define LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H

#include "configs/config.h"
#include "rules_list.h"
#include <stddef.h>

/// @brief Main configuration structure containing all rule configurations.
typedef struct config_s
{
#define X(field, cat_name, sname, rule_ptr, ops_ptr) leuko_rule_config_t *field;
    LEUKO_RULES_LIST
#undef X
    /* AllCops level include/exclude (global defaults) */
    char **all_include;
    size_t all_include_count;
    char **all_exclude;
    size_t all_exclude_count;
} config_t;

void initialize_config(config_t *cfg);
void free_config(config_t *cfg);
leuko_rule_config_t *get_rule_config_by_index(config_t *cfg, size_t idx);
size_t config_count(void);

#endif /* LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H */
