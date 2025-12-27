#ifndef LEUKOCYTE_CONFIGS_RULE_CONFIG_H
#define LEUKOCYTE_CONFIGS_RULE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <regex.h>
#include "common/severity.h"
#include "sources/node.h"

/* Config Keys */
#define LEUKO_GENERAL "general"
#define LEUKO_CONFIG_KEY_ENABLED "Enabled"
#define LEUKO_CONFIG_KEY_SEVERITY "Severity"
#define LEUKO_CONFIG_KEY_INCLUDE "Include"
#define LEUKO_CONFIG_KEY_EXCLUDE "Exclude"

/**
 * @brief Rule configuration base structure.
 */
typedef struct leuko_config_rule_base_s
{
    bool enabled;
    leuko_severity_t severity;
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;
    /* Precompiled regex for include/exclude */
    regex_t *include_re;
    size_t include_re_count;
    regex_t *exclude_re;
    size_t exclude_re_count;
} leuko_config_rule_base_t;

/**
 * @brief Rule configuration view structure including base and specific config.
 */
typedef struct leuko_config_rule_view_s
{
    leuko_config_rule_base_t base;
} leuko_config_rule_view_t;

//* Forward-declare merged node type to avoid heavy includes */
typedef struct leuko_node_s leuko_node_t;
/* Forward-declare top-level config to avoid header cycles */
typedef struct leuko_config_s leuko_config_t;

/**
 * @brief Rule configuration handlers structure.
 */
typedef struct leuko_rule_config_handlers_s
{
    leuko_config_rule_view_t *(*initialize)(void);
    bool (*apply)(leuko_config_rule_view_t *config, leuko_node_t *node, char **err);
} leuko_rule_config_handlers_t;

leuko_config_rule_view_t *leuko_rule_config_initialize(void);
void leuko_rule_config_free(leuko_config_rule_view_t *cfg);
/* Reset an embedded rule struct without freeing the struct itself */
void leuko_rule_config_reset(leuko_config_rule_view_t *cfg);

/* Base helpers: operate on embedded base structs */
void leuko_rule_config_base_reset(leuko_config_rule_base_t *base);
/* Reset a full view (base + specific) */
void leuko_rule_config_view_reset(leuko_config_rule_view_t *view);
/* Move heap-allocated rule (initializer result) into embedded view and free source */
void leuko_rule_config_move_to_view(leuko_config_rule_view_t *src, leuko_config_rule_view_t *dst);

/* Forward-declare registry entry to avoid header cycles */
struct leuko_registry_rule_entry_s;

/* Apply a rule node into the provided rule config and invoke rule-specific handler */
bool leuko_rule_config_apply(leuko_config_rule_view_t *rconf, const struct leuko_registry_rule_entry_s *ent, leuko_node_t *rule_node, char **err);

#endif /* LEUKOCYTE_CONFIGS_RULE_CONFIG_H */
