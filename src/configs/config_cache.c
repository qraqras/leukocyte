#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include "configs/config_cache.h"
#include "utils/string_array.h"
#include "cli/exit_code.h"
#include "cJSON.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Helper: strdup with NULL check */
static char *leuko_dup_or_null(const char *s)
{
    if (!s)
        return NULL;
    char *d = strdup(s);
    return d;
}

/* Extract directory part of a path (caller frees) */
static char *leuko_dirname_dup(const char *path)
{
    if (!path)
        return NULL;
    const char *p = strrchr(path, '/');
    if (!p)
    {
        /* no directory part; return '.' */
        return strdup(".");
    }
    size_t len = (size_t)(p - path);
    char *d = malloc(len + 1);
    if (!d)
        return NULL;
    memcpy(d, path, len);
    d[len] = '\0';
    return d;
}

/* Parse excludes from resolved JSON file into array (allocated via leuko_str_arr_push)
 * returns true on success (even if no excludes)
 */
static bool leuko_parse_excludes_from_resolved(const char *resolved_path, char ***out_excl, size_t *out_count, char ***out_incl, size_t *out_incl_count)
{
    *out_excl = NULL;
    *out_count = 0;
    *out_incl = NULL;
    *out_incl_count = 0;

    FILE *f = fopen(resolved_path, "r");
    if (!f)
    {
        fprintf(stderr, "Warning: could not open resolved config %s\n", resolved_path);
        return false;
    }
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return false;
    }
    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return false;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return false;
    }

    char *buf = malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return false;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz)
    {
        free(buf);
        fclose(f);
        return false;
    }
    buf[sz] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    if (!root)
    {
        free(buf);
        return false;
    }

    /* general.include/exclude */
    cJSON *general = cJSON_GetObjectItemCaseSensitive(root, "general");
    if (cJSON_IsObject(general))
    {
        cJSON *gex = cJSON_GetObjectItemCaseSensitive(general, "exclude");
        if (cJSON_IsArray(gex))
        {
            cJSON *it = NULL;
            cJSON_ArrayForEach(it, gex)
            {
                if (cJSON_IsString(it) && it->valuestring)
                    leuko_str_arr_push(out_excl, out_count, it->valuestring);
            }
        }
        cJSON *ginc = cJSON_GetObjectItemCaseSensitive(general, "include");
        if (cJSON_IsArray(ginc))
        {
            cJSON *it2 = NULL;
            cJSON_ArrayForEach(it2, ginc)
            {
                if (cJSON_IsString(it2) && it2->valuestring)
                    leuko_str_arr_push(out_incl, out_incl_count, it2->valuestring);
            }
        }
    }

    /* categories.*.exclude and rules.*.exclude */
    cJSON *cats = cJSON_GetObjectItemCaseSensitive(root, "categories");
    if (cJSON_IsObject(cats))
    {
        cJSON *cat = NULL;
        cJSON_ArrayForEach(cat, cats)
        {
            if (!cJSON_IsObject(cat))
                continue;
            /* category-level include */
            cJSON *cinc = cJSON_GetObjectItemCaseSensitive(cat, "include");
            if (cJSON_IsArray(cinc))
            {
                cJSON *it = NULL;
                cJSON_ArrayForEach(it, cinc)
                {
                    if (cJSON_IsString(it) && it->valuestring)
                        leuko_str_arr_push(out_incl, out_incl_count, it->valuestring);
                }
            }

            cJSON *cex = cJSON_GetObjectItemCaseSensitive(cat, "exclude");
            if (cJSON_IsArray(cex))
            {
                cJSON *it = NULL;
                cJSON_ArrayForEach(it, cex)
                {
                    if (cJSON_IsString(it) && it->valuestring)
                        leuko_str_arr_push(out_excl, out_count, it->valuestring);
                }
            }
            cJSON *rules = cJSON_GetObjectItemCaseSensitive(cat, "rules");
            if (cJSON_IsObject(rules))
            {
                cJSON *r = NULL;
                cJSON_ArrayForEach(r, rules)
                {
                    if (!cJSON_IsObject(r))
                        continue;
                    /* rule-level include */
                    cJSON *rinc = cJSON_GetObjectItemCaseSensitive(r, "include");
                    if (cJSON_IsArray(rinc))
                    {
                        cJSON *it3 = NULL;
                        cJSON_ArrayForEach(it3, rinc)
                        {
                            if (cJSON_IsString(it3) && it3->valuestring)
                                leuko_str_arr_push(out_incl, out_incl_count, it3->valuestring);
                        }
                    }
                    cJSON *rex = cJSON_GetObjectItemCaseSensitive(r, "exclude");
                    if (cJSON_IsArray(rex))
                    {
                        cJSON *it2 = NULL;
                        cJSON_ArrayForEach(it2, rex)
                        {
                            if (cJSON_IsString(it2) && it2->valuestring)
                                leuko_str_arr_push(out_excl, out_count, it2->valuestring);
                        }
                    }
                }
            }
        }
    }

    /* categories: includes & excludes handled above */
    cJSON_Delete(root);
    free(buf);
    return true;
}

/* Load index.json and build cache */
bool leuko_config_cache_load(const char *index_path, leuko_config_cache_t **out_cache)
{
    if (!out_cache)
        return false;
    *out_cache = NULL;

    const char *idx = index_path ? index_path : ".leukocyte/index.json";
    FILE *f = fopen(idx, "r");
    if (!f)
    {
        fprintf(stderr, "Warning: could not open index %s\n", idx);
        /* Empty cache is acceptable */
        leuko_config_cache_t *empty = calloc(1, sizeof(leuko_config_cache_t));
        if (!empty)
            return false;
        *out_cache = empty;
        return true;
    }
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return false;
    }
    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return false;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return false;
    }

    char *buf = malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return false;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz)
    {
        free(buf);
        fclose(f);
        return false;
    }
    buf[sz] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    if (!root || !cJSON_IsArray(root))
    {
        free(buf);
        if (root)
            cJSON_Delete(root);
        return false;
    }

    leuko_config_cache_t *cache = calloc(1, sizeof(leuko_config_cache_t));
    if (!cache)
    {
        cJSON_Delete(root);
        free(buf);
        return false;
    }

    size_t arr_sz = (size_t)cJSON_GetArraySize(root);
    for (size_t i = 0; i < arr_sz; ++i)
    {
        cJSON *it = cJSON_GetArrayItem(root, (int)i);
        if (!cJSON_IsObject(it))
            continue;
        cJSON *src = cJSON_GetObjectItemCaseSensitive(it, "src");
        cJSON *outp = cJSON_GetObjectItemCaseSensitive(it, "out");
        if (!cJSON_IsString(src) || !cJSON_IsString(outp))
            continue;
        leuko_resolved_config_t *rc = calloc(1, sizeof(leuko_resolved_config_t));
        if (!rc)
            goto error;
        rc->src = leuko_dup_or_null(src->valuestring);
        rc->out = leuko_dup_or_null(outp->valuestring);
        rc->src_dir = leuko_dirname_dup(rc->src);
        rc->excludes = NULL;
        rc->exclude_count = 0;
        rc->includes = NULL;
        rc->include_count = 0;

        /* parse resolved json for includes and excludes */
        leuko_parse_excludes_from_resolved(rc->out, &rc->excludes, &rc->exclude_count, &rc->includes, &rc->include_count);

        /* append to cache */
        leuko_resolved_config_t **tmp = realloc(cache->items, (cache->count + 1) * sizeof(leuko_resolved_config_t *));
        if (!tmp)
        {
            free(rc);
            goto error;
        }
        cache->items = tmp;
        cache->items[cache->count++] = rc;
    }

    cJSON_Delete(root);
    free(buf);

    *out_cache = cache;
    return true;

error:
    if (cache)
    {
        if (cache->items)
        {
            for (size_t j = 0; j < cache->count; ++j)
            {
                leuko_resolved_config_t *r = cache->items[j];
                if (!r)
                    continue;
                free(r->src);
                free(r->out);
                free(r->src_dir);
                if (r->excludes)
                {
                    for (size_t k = 0; k < r->exclude_count; ++k)
                        free(r->excludes[k]);
                    free(r->excludes);
                }
                if (r->includes)
                {
                    for (size_t k = 0; k < r->include_count; ++k)
                        free(r->includes[k]);
                    free(r->includes);
                }
                free(r);
            }
            free(cache->items);
        }
        free(cache);
    }
    if (root)
        cJSON_Delete(root);
    free(buf);
    return false;
}

void leuko_config_cache_free(leuko_config_cache_t *cache)
{
    if (!cache)
        return;
    if (cache->items)
    {
        for (size_t i = 0; i < cache->count; ++i)
        {
            leuko_resolved_config_t *r = cache->items[i];
            if (!r)
                continue;
            free(r->src);
            free(r->out);
            free(r->src_dir);
            if (r->excludes)
            {
                for (size_t k = 0; k < r->exclude_count; ++k)
                    free(r->excludes[k]);
                free(r->excludes);
            }
            if (r->includes)
            {
                for (size_t k = 0; k < r->include_count; ++k)
                    free(r->includes[k]);
                free(r->includes);
            }
            free(r);
        }
        free(cache->items);
    }
    free(cache);
}

leuko_resolved_config_t *leuko_config_cache_find_for_path(leuko_config_cache_t *cache, const char *path)
{
    if (!cache || !path)
        return NULL;
    size_t best_len = 0;
    leuko_resolved_config_t *best = NULL;
    for (size_t i = 0; i < cache->count; ++i)
    {
        leuko_resolved_config_t *r = cache->items[i];
        if (!r || !r->src_dir)
            continue;
        size_t dlen = strlen(r->src_dir);
        if (dlen == 0)
            continue;
        if (strncmp(path, r->src_dir, dlen) == 0)
        {
            /* ensure boundary match: either path[dlen] == '/' or path ends */
            if (path[dlen] == '\0' || path[dlen] == '/')
            {
                if (dlen > best_len)
                {
                    best_len = dlen;
                    best = r;
                }
            }
        }
    }
    return best;
}
