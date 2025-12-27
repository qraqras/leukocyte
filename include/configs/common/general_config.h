#ifndef LEUKOCYTE_CONFIGS_GENERAL_CONFIG_H
#define LEUKOCYTE_CONFIGS_GENERAL_CONFIG_H

#include <stddef.h>
#include <regex.h>
#include <stdbool.h>
#include "configs/common/rule_config.h"

/**
 * @brief General configuration structure.
 */
typedef struct leuko_config_general_s
{
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;
    /* Precompiled regex patterns for fast matching (prototype) */
    regex_t *include_re;
    size_t include_re_count;
    regex_t *exclude_re;
    size_t exclude_re_count;
} leuko_config_general_t;

typedef struct leuko_config_s leuko_config_t;
typedef struct leuko_node_s leuko_node_t;

leuko_config_general_t *leuko_general_config_initialize(void);
void leuko_general_config_free(leuko_config_general_t *cfg);
bool leuko_general_config_apply(leuko_config_t *cfg, leuko_node_t *general);

#endif /* LEUKOCYTE_CONFIGS_GENERAL_CONFIG_H */
