#ifndef LEUKO_CONFIGS_CONFIG_CACHE_H
#define LEUKO_CONFIGS_CONFIG_CACHE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct leuko_resolved_config_s
{
    char *src;            /* path to source .rubocop.yml */
    char *out;            /* path to resolved json */
    char *src_dir;        /* directory containing src (for path-based matching) */
    char **excludes;      /* array of exclude patterns */
    size_t exclude_count; /* number of exclude patterns */
    char **includes;      /* array of include patterns */
    size_t include_count; /* number of include patterns */
} leuko_resolved_config_t;

typedef struct leuko_config_cache_s
{
    leuko_resolved_config_t **items;
    size_t count;
} leuko_config_cache_t;

/*
 * Load resolved configs referenced by index_path (default ".leukocyte/index.json")
 * On success returns true and sets *out_cache to a newly allocated cache (caller frees it).
 */
bool leuko_config_cache_load(const char *index_path, leuko_config_cache_t **out_cache);

/* Free a cache previously returned by leuko_config_cache_load */
void leuko_config_cache_free(leuko_config_cache_t *cache);

/*
 * Find the most specific resolved config applicable to `path`.
 * Returns NULL if none matched.
 */
leuko_resolved_config_t *leuko_config_cache_find_for_path(leuko_config_cache_t *cache, const char *path);

#endif /* LEUKO_CONFIGS_CONFIG_CACHE_H */
