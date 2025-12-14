#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations for config-related opaque types
typedef struct pattern_list_s pattern_list_t;
typedef struct rule_config_s rule_config_t;
typedef struct config_s config_t;

// Rule config API (prototypes)
rule_config_t *rule_config_create(const char *name);
void rule_config_set_enabled(rule_config_t *r, bool enabled);
void rule_config_set_severity(rule_config_t *r, int severity);
void rule_config_set_typed(rule_config_t *r, void *typed, void (*free_fn)(void *));

// Pattern helpers
pattern_list_t *pattern_list_add(pattern_list_t **head, const char *pattern);
void pattern_list_free(pattern_list_t *list);
bool config_matches_path(pattern_list_t *patterns, const char *path);

// Config lifecycle
config_t *config_load_from_file(const char *path);
void config_free(config_t *cfg);

#endif // LEUKOCYTE_CONFIGS_CONFIG_H
