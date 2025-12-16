#ifndef LEUKOCYTE_CONFIGS_CONFIG_H
#define LEUKOCYTE_CONFIGS_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "severity.h"

typedef struct
{
    bool enabled;
    severity_level_t severity_level;
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;
    void *specific_config;
    void (*specific_config_free)(void *);
} rule_config_t;

#endif /* LEUKOCYTE_CONFIGS_CONFIG_H */
