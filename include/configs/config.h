#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "severity.h"

/* Configs */
/* clang-format off */
#define ALL_COPS            "AllCops"
#define INHERIT_FROM        "inherit_from"
#define CONFIG_KEY_ENABLED  "Enabled"
#define CONFIG_KEY_SEVERITY "Severity"
#define CONFIG_KEY_INCLUDE  "Include"
#define CONFIG_KEY_EXCLUDE  "Exclude"
/* clang-format on */

typedef struct rule_config_s
{
    bool enabled;
    leuko_severity_level_t severity_level;
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;
    void *specific_config;
    void (*specific_config_free)(void *);
} rule_config_t;

#endif /* LEUKOCYTE_CONFIGS_CONFIG_H */
