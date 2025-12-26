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
 * @brief Rule configuration structure.
 */
typedef struct leuko_rule_config_s
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

    void *specific_config;
    void (*specific_config_free)(void *);
} leuko_rule_config_t;

/* Forward declare merged node type to avoid heavy includes */
typedef struct leuko_node_s leuko_node_t;

/**
 * @brief Rule configuration handlers structure.
 */
typedef struct leuko_rule_config_handlers_s
{
    leuko_rule_config_t *(*initialize)(void);
    bool (*apply)(leuko_rule_config_t *config, leuko_node_t *node, char **err);
} leuko_rule_config_handlers_t;

leuko_rule_config_t *leuko_rule_config_initialize(void);
void leuko_rule_config_free(leuko_rule_config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_RULE_CONFIG_H */
