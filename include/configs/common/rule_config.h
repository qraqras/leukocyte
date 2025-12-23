#ifndef LEUKOCYTE_CONFIGS_RULE_CONFIG_H
#define LEUKOCYTE_CONFIGS_RULE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <yaml.h>
#include "prism.h"
#include "common/severity.h"

/* Config Keys */
#define LEUKO_ALL_COPS "AllCops"
#define LEUKO_INHERIT_FROM "inherit_from"
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
    void *specific_config;
    void (*specific_config_free)(void *);
} leuko_rule_config_t;

/**
 * @brief Rule configuration handlers structure.
 */
typedef struct leuko_rule_config_handlers_s
{
    leuko_rule_config_t *(*initialize)(void);
    bool (*apply_yaml)(leuko_rule_config_t *config, yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, char **err);
} leuko_rule_config_handlers_t;

leuko_rule_config_t *leuko_rule_config_initialize(void);

#endif /* LEUKOCYTE_CONFIGS_RULE_CONFIG_H */
