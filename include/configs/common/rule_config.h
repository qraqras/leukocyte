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

typedef struct leuko_node_s leuko_node_t;
typedef struct leuko_config_s leuko_config_t;

/**
 * @brief Rule configuration handlers structure.
 */
typedef struct leuko_rule_config_handlers_s
{
    /* initialize returns a typed heap view pointer (void*), rule will cast as-needed */
    void *(*initialize)(void);
    /* apply/reset accept void* typed view */
    bool (*apply)(void *view, leuko_node_t *node, char **err);
    void (*reset)(void *view);
} leuko_rule_config_handlers_t;

void *leuko_rule_config_initialize(void);
void leuko_rule_config_free(void *cfg);
void leuko_rule_config_reset(void *cfg);
void leuko_rule_config_base_reset(leuko_config_rule_base_t *base);
void leuko_rule_config_view_reset(void *view);
void leuko_rule_config_move_to_view(void *src, void *dst);

struct leuko_registry_rule_entry_s;

bool leuko_rule_config_apply(void *rconf, const struct leuko_registry_rule_entry_s *ent, leuko_node_t *rule_node, char **err);

#endif /* LEUKOCYTE_CONFIGS_RULE_CONFIG_H */
