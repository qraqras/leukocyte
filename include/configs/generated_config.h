/* Generated config structure from RULES_LIST fields */
#ifndef LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H
#define LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H

#include "configs/config.h"
#include "rules_list.h"

/* Define the aggregate config struct where each RULES_LIST field becomes a member */
typedef struct config_s
{
#define X(field, cat, name, rule_ptr, ops_ptr) rule_config_t *field;
    RULES_LIST
#undef X
} config_t;

/* Initialize all fields to their default configs (calls per-rule ops->create_default) */
void config_init_defaults(config_t *cfg);

/* Free all configs and owned memory */
void config_free(config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H */
