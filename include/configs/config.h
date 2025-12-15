#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include "configs/severity.h"

// Forward declarations for config-related opaque types
typedef struct pattern_list_s pattern_list_t;
typedef struct rule_config_s rule_config_t;
typedef struct config_s config_t;

// Rule config API (prototypes)
rule_config_t *rule_config_create(const char *name);
void rule_config_set_enabled(rule_config_t *r, bool enabled);
void rule_config_set_severity(rule_config_t *r, severity_level_t severity);
void rule_config_set_typed(rule_config_t *r, void *typed, void (*free_fn)(void *));
void rule_config_free(rule_config_t *r);
// Accessors
bool rule_config_get_enabled(rule_config_t *r);
int rule_config_get_severity(rule_config_t *r);
void *rule_config_get_typed(rule_config_t *r);
// Helpers to add patterns
void add_rule_config_include(rule_config_t *r, const char *pattern);
void add_rule_config_exclude(rule_config_t *r, const char *pattern);

// Accessors for include/exclude lists
size_t rule_config_get_include_count(rule_config_t *r);
const char *rule_config_get_include_at(rule_config_t *r, size_t idx);
size_t rule_config_get_exclude_count(rule_config_t *r);
const char *rule_config_get_exclude_at(rule_config_t *r, size_t idx);

// Find rule by name in config
rule_config_t *config_find_rule(config_t *cfg, const char *name);

// Pattern helpers
pattern_list_t *pattern_list_add(pattern_list_t **head, const char *pattern);
void pattern_list_free(pattern_list_t *list);
bool config_matches_path(pattern_list_t *patterns, const char *path);

// Config lifecycle
config_t *config_load_from_file(const char *path);
void config_free(config_t *cfg);
// Add a rule to the config linked list
void config_add_rule(config_t *cfg, rule_config_t *r);
// Add all built-in default rules to the given config
void config_add_default_rules(config_t *cfg);

#endif // LEUKOCYTE_CONFIGS_CONFIG_H
