#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "configs/discovery/inherit.h"

/* Simple mtime-based cache: maps base canonical path -> list of parent canonical paths + mtimes */
typedef struct
{
    char *base_canon;
    char **parent_paths; /* canonical parent file paths */
    time_t *parent_mtimes;
    leuko_raw_config_t **parent_cfgs; /* cached parsed parents (refs owned by cache) */
    size_t parent_count;
    time_t base_mtime;
} inherit_cache_entry_t;

static inherit_cache_entry_t *g_cache = NULL;
static size_t g_cache_count = 0;
static size_t g_cache_cap = 0;

/* Helper: get file mtime, return -1 on error */
static time_t get_mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return (time_t)-1;
    return st.st_mtime;
}

/* Try to get cached parent list. If valid, load leuko_raw_config_t for each parent and return them via out_parents.
 * Returns 0 on success (even if no parents), 1 on cache-miss or invalidation, and sets *err on serious errors.
 */
int leuko_inherit_cache_try_get(const char *base_path, leuko_raw_config_t ***out_parents, size_t *out_parent_count, char **err)
{
    if (!base_path || !out_parents || !out_parent_count)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }

    char *canon = realpath(base_path, NULL);
    if (!canon)
        canon = strdup(base_path);

    time_t base_m = get_mtime(canon);

    for (size_t i = 0; i < g_cache_count; i++)
    {
        if (strcmp(g_cache[i].base_canon, canon) == 0)
        {
            /* verify mtimes */
            if (g_cache[i].base_mtime != base_m)
            {
                free(canon);
                return 1; /* invalid */
            }
            for (size_t j = 0; j < g_cache[i].parent_count; j++)
            {
                time_t pm = get_mtime(g_cache[i].parent_paths[j]);
                if (pm != g_cache[i].parent_mtimes[j])
                {
                    free(canon);
                    return 1; /* invalid */
                }
            }

            /* Cache valid: return cached parsed parents (increase refs for caller) */
            leuko_raw_config_t **res = calloc(g_cache[i].parent_count, sizeof(leuko_raw_config_t *));
            if (!res)
            {
                if (err)
                    *err = strdup("allocation failure");
                free(canon);
                return 1;
            }
            for (size_t j = 0; j < g_cache[i].parent_count; j++)
            {
                /* bump ref for caller */
                leuko_raw_config_ref(g_cache[i].parent_cfgs[j]);
                res[j] = g_cache[i].parent_cfgs[j];
            }
            *out_parents = res;
            *out_parent_count = g_cache[i].parent_count;
            free(canon);
            return 0;
        }
    }

    free(canon);
    return 1;
}

int leuko_inherit_cache_put(const char *base_path, leuko_raw_config_t **parents, size_t parent_count)
{
    if (!base_path)
        return 1;

    char *canon = realpath(base_path, NULL);
    if (!canon)
        canon = strdup(base_path);

    /* Collect canonical parent paths and mtimes */
    char **ppaths = NULL;
    time_t *pmt = NULL;
    leuko_raw_config_t **pcfgs = NULL;
    if (parent_count > 0)
    {
        ppaths = calloc(parent_count, sizeof(char *));
        pmt = calloc(parent_count, sizeof(time_t));
        pcfgs = calloc(parent_count, sizeof(leuko_raw_config_t *));
        if (!ppaths || !pmt || !pcfgs)
        {
            free(ppaths);
            free(pmt);
            free(pcfgs);
            free(canon);
            return 1;
        }
        for (size_t i = 0; i < parent_count; i++)
        {
            char *p = parents[i]->path;
            char *pc = realpath(p, NULL);
            if (!pc)
                pc = strdup(p);
            ppaths[i] = pc;
            pmt[i] = get_mtime(pc);
            /* cache should own a reference */
            leuko_raw_config_ref(parents[i]);
            pcfgs[i] = parents[i];
        }
    }

    time_t base_m = get_mtime(canon);

    /* Add or replace entry */
    for (size_t i = 0; i < g_cache_count; i++)
    {
        if (strcmp(g_cache[i].base_canon, canon) == 0)
        {
            /* replace: free old entries (paths + mtimes + cfg refs) */
            for (size_t j = 0; j < g_cache[i].parent_count; j++)
                free(g_cache[i].parent_paths[j]);
            free(g_cache[i].parent_paths);
            free(g_cache[i].parent_mtimes);
            if (g_cache[i].parent_cfgs)
            {
                for (size_t j = 0; j < g_cache[i].parent_count; j++)
                    leuko_raw_config_unref(g_cache[i].parent_cfgs[j]);
                free(g_cache[i].parent_cfgs);
            }
            g_cache[i].parent_paths = ppaths;
            g_cache[i].parent_mtimes = pmt;
            g_cache[i].parent_cfgs = pcfgs;
            g_cache[i].parent_count = parent_count;
            g_cache[i].base_mtime = base_m;
            free(canon);
            return 0;
        }
    }

    /* append new */
    if (g_cache_count == g_cache_cap)
    {
        size_t ncap = g_cache_cap == 0 ? 8 : g_cache_cap * 2;
        inherit_cache_entry_t *n = realloc(g_cache, ncap * sizeof(inherit_cache_entry_t));
        if (!n)
        {
            for (size_t j = 0; j < parent_count; j++)
                free(ppaths[j]);
            free(ppaths);
            free(pmt);
            free(canon);
            return 1;
        }
        g_cache = n;
        g_cache_cap = ncap;
    }
    g_cache[g_cache_count].base_canon = canon;
    g_cache[g_cache_count].parent_paths = ppaths;
    g_cache[g_cache_count].parent_mtimes = pmt;
    g_cache[g_cache_count].parent_cfgs = pcfgs;
    g_cache[g_cache_count].parent_count = parent_count;
    g_cache[g_cache_count].base_mtime = base_m;
    g_cache_count++;
    return 0;
}

void leuko_inherit_cache_clear(void)
{
    for (size_t i = 0; i < g_cache_count; i++)
    {
        free(g_cache[i].base_canon);
        for (size_t j = 0; j < g_cache[i].parent_count; j++)
            free(g_cache[i].parent_paths[j]);
        free(g_cache[i].parent_paths);
        free(g_cache[i].parent_mtimes);
        if (g_cache[i].parent_cfgs)
        {
            for (size_t j = 0; j < g_cache[i].parent_count; j++)
                leuko_raw_config_unref(g_cache[i].parent_cfgs[j]);
            free(g_cache[i].parent_cfgs);
        }
    }
    free(g_cache);
    g_cache = NULL;
    g_cache_count = 0;
    g_cache_cap = 0;
}
