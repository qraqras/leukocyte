#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stddef.h>
#include "common/registry/list.h"
#include "configs/common/rule_config.h"
#include "configs/common/general_config.h"
#include "configs/common/category_config.h"

#define LEUKO_INHERIT_FROM "inherit_from"
#define LEUKO_INHERIT_MODE "inherit_mode"
#define LEUKO_INHERIT_MODE_MERGE "merge"
#define LEUKO_INHERIT_MODE_OVERRIDE "override"



/**
 * @brief Main configuration structure containing all rule configurations.
 */
typedef struct leuko_config_s
{
    leuko_config_general_t *general;
    leuko_config_categories_view_t categories; /* access: cfg->categories.layout.rules.indentation_consistency */
} leuko_config_t;

/* Helpers for general/category access */
leuko_config_general_t *leuko_config_get_general_config(leuko_config_t *cfg);
leuko_config_category_t *leuko_config_get_category_config(leuko_config_t *cfg, const char *name);
leuko_config_category_view_t *leuko_config_get_view_category(leuko_config_t *cfg, const char *name);

/* Helper: access a rule base by category and rule name (e.g., categories.Layout.rules.TrailingWhitespace) */
leuko_config_rule_base_t *leuko_config_get_rule(leuko_config_t *cfg, const char *category, const char *rule_name);
leuko_config_rule_view_t *leuko_config_get_view_rule(leuko_config_t *cfg, const char *category, const char *rule_name);
void leuko_config_set_view_rule(leuko_config_t *cfg, const char *category, const char *rule_name, leuko_config_rule_view_t *rconf);
void leuko_config_initialize(leuko_config_t *cfg);
void leuko_config_free(leuko_config_t *cfg);
size_t leuko_config_count(void);

#endif /* LEUKOCYTE_CONFIGS_CONFIG_H */
