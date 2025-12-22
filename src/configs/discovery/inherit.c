/*
 * Resolve `inherit_from` entries and load inherited YAML configs.
 */

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <glob.h>
#include <stdio.h>

#include "configs/discovery/inherit.h"
#include "configs/loader/yaml_helpers.h"
#include <sys/stat.h>

/* Helper: join base dir and relative path (allocates returned string) */
static char *join_base_and_path(const char *base_dir, const char *path)
{
    if (!base_dir || !path)
    {
        return NULL;
    }
    /* If path is absolute, return a duplicate */
    if (path[0] == '/')
    {
        return strdup(path);
    }

    size_t bl = strlen(base_dir);
    size_t pl = strlen(path);
    size_t total = bl + 1 + pl + 1;
    char *res = malloc(total);
    if (!res)
    {
        return NULL;
    }
    memcpy(res, base_dir, bl);
    res[bl] = '/';
    memcpy(res + bl + 1, path, pl);
    res[total - 1] = '\0';
    return res;
}

/* Helper: get canonical path (realpath if possible, else duplicate) */
static char *canonical_path(const char *path)
{
    if (!path)
        return NULL;
    char *r = realpath(path, NULL);
    if (r)
        return r;
    return strdup(path);
}

/* Internal recursive collector helper */
static int collect_recursive(const leuko_raw_config_t *base, leuko_raw_config_t ***acc, size_t *acc_count, char ***seen, size_t *seen_count, char **err)
{
    if (!base || !acc || !acc_count || !seen || !seen_count)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }

    /* Check for cycle via canonical path */
    char *canon = canonical_path(base->path);
    for (size_t i = 0; i < *seen_count; i++)
    {
        if (strcmp((*seen)[i], canon) == 0)
        {
            free(canon);
            if (err)
                *err = strdup("inherit cycle detected");
            return 1;
        }
    }
    /* Mark seen */
    char **s2 = realloc(*seen, (*seen_count + 1) * sizeof(char *));
    if (!s2)
    {
        free(canon);
        if (err)
            *err = strdup("allocation failure");
        return 1;
    }
    *seen = s2;
    (*seen)[*seen_count] = canon;
    (*seen_count)++;

    /* Get direct parents */
    leuko_raw_config_t **parents = NULL;
    size_t parent_count = 0;
    char *local_err = NULL;
    int rc = leuko_config_resolve_inherit_from(base, &parents, &parent_count, &local_err);
    if (rc != 0)
    {
        if (local_err)
        {
            if (err)
                *err = strdup(local_err);
            free(local_err);
        }
        return 1;
    }

    /* Recurse into parents first (so parents are earlier in final order) */
    for (size_t i = 0; i < parent_count; i++)
    {
        if (collect_recursive(parents[i], acc, acc_count, seen, seen_count, err) != 0)
        {
            /* propagate error; parents[i] and remaining parents will be freed below */
            for (size_t j = 0; j < parent_count; j++)
                leuko_raw_config_free(parents[j]);
            free(parents);
            return 1;
        }
    }

    /* Append direct parents in order they were found (but avoid duplicates) */
    for (size_t i = 0; i < parent_count; i++)
    {
        /* If this parent is already in acc (by canonical path), skip */
        char *pcanon = canonical_path(parents[i]->path);
        bool found = false;
        for (size_t k = 0; k < *acc_count; k++)
        {
            if (strcmp((*acc)[k]->path, parents[i]->path) == 0 || strcmp((*acc)[k]->path, pcanon) == 0)
            {
                found = true;
                break;
            }
        }
        free(pcanon);
        if (!found)
        {
            leuko_raw_config_t **tmp = realloc(*acc, (*acc_count + 1) * sizeof(leuko_raw_config_t *));
            if (!tmp)
            {
                if (err)
                    *err = strdup("allocation failure");
                for (size_t j = 0; j < parent_count; j++)
                    leuko_raw_config_free(parents[j]);
                free(parents);
                return 1;
            }
            *acc = tmp;
            (*acc)[*acc_count] = parents[i];
            (*acc_count)++;
        }
        else
        {
            leuko_raw_config_free(parents[i]);
        }
    }
    free(parents);
    return 0;
}

int leuko_config_collect_inherit_chain(const leuko_raw_config_t *base, leuko_raw_config_t ***out, size_t *out_count, char **err)
{
    if (!base || !out || !out_count)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }

    *out = NULL;
    *out_count = 0;

    char **seen = NULL;
    size_t seen_count = 0;
    leuko_raw_config_t **acc = NULL;
    size_t acc_count = 0;

    /* Try fast path: check cache for resolved parent list (avoid re-globbing) */
    /* Cache entries store parent canonical paths and mtimes and base mtime */
    extern int leuko_inherit_cache_try_get(const char *base_path, leuko_raw_config_t ***out_parents, size_t *out_parent_count, char **err);
    int rc = leuko_inherit_cache_try_get(base->path, &acc, &acc_count, err);
    if (rc == 0 && acc_count > 0)
    {
        /* Cached: return parsed parents (loaded by cache helper) */
        *out = acc;
        *out_count = acc_count;
        /* cleanup seen */
        for (size_t i = 0; i < seen_count; i++)
            free(seen[i]);
        free(seen);
        return 0;
    }

    /* Fallback: full recursive resolution */
    rc = collect_recursive(base, &acc, &acc_count, &seen, &seen_count, err);

    /* cleanup seen */
    for (size_t i = 0; i < seen_count; i++)
        free(seen[i]);
    free(seen);

    if (rc != 0)
    {
        /* cleanup acc */
        if (acc)
        {
            for (size_t i = 0; i < acc_count; i++)
                leuko_raw_config_free(acc[i]);
            free(acc);
        }
        return rc;
    }

    *out = acc;
    *out_count = acc_count;

    /* Store resolved parent list in cache (only parent paths and mtimes) */
    extern int leuko_inherit_cache_put(const char *base_path, leuko_raw_config_t **parents, size_t parent_count);
    leuko_inherit_cache_put(base->path, acc, acc_count);
    return 0;
}

void leuko_raw_config_list_free(leuko_raw_config_t **list, size_t count)
{
    if (!list)
        return;
    for (size_t i = 0; i < count; i++)
    {
        leuko_raw_config_unref(list[i]);
    }
    free(list);
}

int leuko_config_resolve_inherit_from(const leuko_raw_config_t *base, leuko_raw_config_t ***out, size_t *out_count, char **err)
{
    if (!base || !out || !out_count)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }

    yaml_document_t *doc = base->doc;
    if (!doc)
    {
        if (err)
            *err = strdup("no document");
        return 1;
    }

    yaml_node_t *root = yaml_document_get_root_node(doc);
    if (!root || root->type != YAML_MAPPING_NODE)
    {
        *out = NULL;
        *out_count = 0;
        return 0;
    }

    /* Extract inherit_from as scalar or sequence */
    char *single = yaml_get_mapping_scalar_value(doc, root, "inherit_from");
    size_t patterns_count = 0;
    char **patterns = NULL;

    if (single)
    {
        patterns = malloc(sizeof(char *));
        if (!patterns)
        {
            if (err)
                *err = strdup("allocation failure");
            return 1;
        }
        patterns[0] = strdup(single);
        patterns_count = 1;
    }
    else
    {
        patterns = yaml_get_mapping_sequence_values(doc, root, "inherit_from", &patterns_count);
        if (!patterns)
        {
            patterns_count = 0;
        }
    }

    if (patterns_count == 0)
    {
        if (patterns)
            free(patterns);
        *out = NULL;
        *out_count = 0;
        return 0;
    }

    /* Base directory for resolving relative paths */
    char *pathdup = strdup(base->path ? base->path : "");
    char *base_dir = dirname(pathdup);

    leuko_raw_config_t **result = NULL;
    size_t result_count = 0;

    for (size_t i = 0; i < patterns_count; i++)
    {
        char *pat = patterns[i];
        char *resolved = NULL;
        char *to_glob = NULL;
        if (pat[0] == '/')
        {
            to_glob = strdup(pat);
        }
        else
        {
            resolved = join_base_and_path(base_dir, pat);
            if (!resolved)
            {
                if (err)
                    *err = strdup("allocation failure");
                goto fail;
            }
            to_glob = resolved;
        }

        /* Use glob to expand patterns */
        glob_t g;
        memset(&g, 0, sizeof(g));
        int gr = glob(to_glob, 0, NULL, &g);
        if (gr == GLOB_NOMATCH)
        {
            if (err)
                *err = strdup("inherit_from pattern matched no files");
            globfree(&g);
            if (resolved)
                free(resolved);
            goto fail;
        }
        else if (gr != 0)
        {
            if (err)
                *err = strdup("failed to expand inherit_from pattern");
            globfree(&g);
            if (resolved)
                free(resolved);
            goto fail;
        }

        for (size_t m = 0; m < g.gl_pathc; m++)
        {
            const char *p = g.gl_pathv[m];
            leuko_raw_config_t *child = NULL;
            char *child_err = NULL;
            int rc = leuko_config_load_file(p, &child, &child_err);
            if (rc != 0)
            {
                if (child_err)
                {
                    if (err)
                        *err = strdup(child_err);
                    free(child_err);
                }
                else if (err)
                {
                    *err = strdup("failed to load inherited config");
                }
                globfree(&g);
                if (resolved)
                    free(resolved);
                goto fail;
            }
            leuko_raw_config_t **tmp = realloc(result, (result_count + 1) * sizeof(leuko_raw_config_t *));
            if (!tmp)
            {
                if (err)
                    *err = strdup("allocation failure");
                leuko_raw_config_free(child);
                globfree(&g);
                if (resolved)
                    free(resolved);
                goto fail;
            }
            result = tmp;
            result[result_count++] = child;
        }

        globfree(&g);
        if (resolved)
            free(resolved);
    }

    for (size_t i = 0; i < patterns_count; i++)
    {
        free(patterns[i]);
    }
    free(patterns);
    free(pathdup);

    *out = result;
    *out_count = result_count;
    return 0;

fail:
    if (result)
    {
        for (size_t j = 0; j < result_count; j++)
        {
            leuko_raw_config_free(result[j]);
        }
        free(result);
    }
    if (patterns)
    {
        for (size_t i = 0; i < patterns_count; i++)
            free(patterns[i]);
        free(patterns);
    }
    free(pathdup);
    return 1;
}
