#ifndef LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H
#define LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H

#include <stddef.h>

/* Per-category configuration (e.g., Layout) */
#include <regex.h>

typedef struct leuko_category_config_s
{
    char *name; /* category name, e.g., "Layout" */
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;

    /* Precompiled regex patterns */
    regex_t *include_re;
    size_t include_re_count;
    regex_t *exclude_re;
    size_t exclude_re_count;
} leuko_category_config_t;

leuko_category_config_t *leuko_category_config_initialize(const char *name);
void leuko_category_config_free(leuko_category_config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H */
