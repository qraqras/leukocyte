#ifndef LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H
#define LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H

#include "configs/config.h"
#include "rules_list.h"

/// @brief Main configuration structure containing all rule configurations.
typedef struct config_s
{
#define X(field, cat, name, rule_ptr, ops_ptr) rule_config_t *field;
    RULES_LIST
#undef X
} config_t;

void config_initialize(config_t *cfg);
void config_free(config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H */
