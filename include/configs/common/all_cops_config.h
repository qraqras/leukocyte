#ifndef LEUKOCYTE_CONFIGS_ALL_COPS_CONFIG_H
#define LEUKOCYTE_CONFIGS_ALL_COPS_CONFIG_H

#include <stddef.h>

/*
 * Typed representation of AllCops configuration.
 * Kept minimal: only fields needed by current logic.
 */
#include <regex.h>

typedef struct leuko_all_cops_config_s
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
} leuko_all_cops_config_t;

leuko_all_cops_config_t *leuko_all_cops_config_initialize(void);
void leuko_all_cops_config_free(leuko_all_cops_config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_ALL_COPS_CONFIG_H */
