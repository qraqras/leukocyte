#ifndef LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H
#define LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H

#include "configs/config.h"
#include "rules_list.h"
#include <stddef.h>

/// @brief Main configuration structure containing all rule configurations.
typedef struct config_s
{
#define X(field, cat, name, rule_ptr, ops_ptr) rule_config_t *field;
    RULES_LIST
#undef X
} config_t;

void config_initialize(config_t *cfg);
/// @brief Free a config_t structure and its contents.
/// @param cfg If NULL, free the global registry-initialized config created by passing NULL to `config_initialize`.
void config_free(config_t *cfg);

/* Global registry helpers
 * - Call `config_initialize(NULL)` to create the global registry-backed config.
 * - Use `config_get_by_index`/`config_count` to access it.
 * - Call `config_free(NULL)` to free the global instance.
 */
rule_config_t *config_get_by_index(size_t idx);
size_t config_count(void);

#endif /* LEUKOCYTE_CONFIGS_GENERATED_CONFIG_H */
