#ifndef LEUKOCYTE_CONFIGS_GENERAL_CONFIG_H
#define LEUKOCYTE_CONFIGS_GENERAL_CONFIG_H

#include <stddef.h>
#include <regex.h>

/* Global/general configuration (formerly AllCops) */
typedef struct leuko_general_config_s
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
} leuko_general_config_t;

leuko_general_config_t *leuko_general_config_initialize(void);
void leuko_general_config_free(leuko_general_config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_GENERAL_CONFIG_H */
