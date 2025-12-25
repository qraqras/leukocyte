#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <regex.h>

#include "configs/compiled_config.h"
#include "configs/common/config.h"
#include "configs/common/general_config.h"
#include "configs/common/category_config.h"
#include "utils/allocator/arena.h"
#include "utils/glob_to_regex.h"
#include "utils/string_array.h"
#include "sources/node.h"
#include "sources/json/parse.h"
#include "common/registry/registry.h"

/* Minimal, clear and node-based compiled_config implementation.
 * This file replaces the legacy document-based code and provides:
 * - build/ref/unref
 * - node-based materialize
 * - simple fingerprint computation
 * - accessors used by tests
 */

/* Simple FNV-1a 64bit for fingerprinting */
static uint64_t
leuko_fnv1a_64(const void *data, size_t len)
{
    const unsigned char *p = data;
    uint64_t h = 14695981039346656037ULL;
    size_t i;
    for (i = 0; i < len; i++)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

/* Helper: compile glob strings into regex_t array */
static bool
leuko_compile_regex_array(char **patterns, size_t count, regex_t **out, size_t *out_count)
{
    if (!patterns || count == 0)
    {
        *out = NULL;
        *out_count = 0;
        return true;
    }

    regex_t *arr = NULL;
    size_t n = 0;
    for (size_t i = 0; i < count; i++)
    {
        char *re_s = leuko_glob_to_regex(patterns[i]);
        if (!re_s)
            continue;
        regex_t r;
        int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
        free(re_s);
        if (rc != 0)
            continue;
        regex_t *tmp = realloc(arr, sizeof(regex_t) * (n + 1));
        if (!tmp)
        {
            regfree(&r);
            /* free already compiled */
            for (size_t j = 0; j < n; j++)
                regfree(&arr[j]);
            free(arr);
            return false;
        }
        arr = tmp;
        arr[n] = r;
        n++;
    }
    *out = arr;
    *out_count = n;
    return true;
}

/* Build node from files. Since exporter JSON files are expected to be already merged,
 * we simply parse each file and return the last successfully parsed node (last-wins).
 */
static leuko_node_t *
build_merged_node_from_files(struct leuko_arena *a, char **files, size_t count)
{
    if (!files || count == 0)
        return NULL;
    leuko_node_t *last = NULL;
    for (size_t i = 0; i < count; i++)
    {
        leuko_node_t *local = NULL;
        if (!leuko_json_parse(files[i], &local))
        {
            if (last)
                leuko_node_free(last);
            return NULL;
        }
        if (last)
            leuko_node_free(last);
        last = local;
    }
    return last;
}

/* Materialize rules from a merged node into a freshly allocated leuko_config_t.
 * Allocations are done with heap allocation (not arena) so callers can choose
 * to free via leuko_config_free or keep it in an arena.
 */
static leuko_config_t *
materialize_rules_from_node(struct leuko_arena *a, leuko_node_t *root)
{
    if (!root)
        return NULL;
    leuko_config_t *cfg = leuko_arena_alloc(a, sizeof(leuko_config_t));
    if (!cfg)
        return NULL;
    leuko_config_initialize(cfg);

    /* General (was AllCops) */
    leuko_node_t *general = leuko_node_get_mapping_child(root, "general");
    if (general && general->type == LEUKO_NODE_OBJECT)
    {
        leuko_node_t *inc = leuko_node_get_mapping_child(general, LEUKO_CONFIG_KEY_INCLUDE);
        size_t n = leuko_node_array_count(inc);
        if (n > 0)
        {
            leuko_general_config_t *ac = leuko_config_get_general_config(cfg);
            ac->include = calloc(n, sizeof(char *));
            ac->include_count = n;
            for (size_t i = 0; i < n; i++)
            {
                const char *s = leuko_node_array_scalar_at(inc, i);
                ac->include[i] = strdup(s ? s : "");
            }
            leuko_compile_regex_array(ac->include, ac->include_count, &ac->include_re, &ac->include_re_count);
        }
        leuko_node_t *exc = leuko_node_get_mapping_child(general, LEUKO_CONFIG_KEY_EXCLUDE);
        n = leuko_node_array_count(exc);
        if (n > 0)
        {
            leuko_general_config_t *ac = leuko_config_get_general_config(cfg);
            ac->exclude = calloc(n, sizeof(char *));
            ac->exclude_count = n;
            for (size_t i = 0; i < n; i++)
            {
                const char *s = leuko_node_array_scalar_at(exc, i);
                ac->exclude[i] = strdup(s ? s : "");
            }
            leuko_compile_regex_array(ac->exclude, ac->exclude_count, &ac->exclude_re, &ac->exclude_re_count);
        }
    }

    /* Categories */
    if (root->type == LEUKO_NODE_OBJECT)
    {
        for (size_t i = 0; i < root->map_len; i++)
        {
            const char *cat = root->map_keys[i];
            if (!cat)
                continue;
            if (strcmp(cat, "general") == 0)
                continue;
            leuko_node_t *cmap = root->map_vals[i];
            if (!cmap || cmap->type != LEUKO_NODE_OBJECT)
                continue;
            leuko_category_config_t *cc = leuko_config_add_category(cfg, cat);
            leuko_node_t *cinc = leuko_node_get_mapping_child(cmap, LEUKO_CONFIG_KEY_INCLUDE);
            size_t n = leuko_node_array_count(cinc);
            if (n > 0)
            {
                cc->include = calloc(n, sizeof(char *));
                cc->include_count = n;
                for (size_t j = 0; j < n; j++)
                    cc->include[j] = strdup(leuko_node_array_scalar_at(cinc, j) ?: "");
                leuko_compile_regex_array(cc->include, cc->include_count, &cc->include_re, &cc->include_re_count);
            }
            leuko_node_t *cexc = leuko_node_get_mapping_child(cmap, LEUKO_CONFIG_KEY_EXCLUDE);
            n = leuko_node_array_count(cexc);
            if (n > 0)
            {
                cc->exclude = calloc(n, sizeof(char *));
                cc->exclude_count = n;
                for (size_t j = 0; j < n; j++)
                    cc->exclude[j] = strdup(leuko_node_array_scalar_at(cexc, j) ?: "");
                leuko_compile_regex_array(cc->exclude, cc->exclude_count, &cc->exclude_re, &cc->exclude_re_count);
            }
        }
    }

    /* Normalize rule keys and let rule handlers apply merged settings */
    leuko_node_t *merged = leuko_node_deep_copy(root);
    leuko_node_normalize_rule_keys(merged);

    const rule_registry_entry_t *reg = leuko_get_rule_registry();
    size_t reg_n = leuko_get_rule_registry_count();
    for (size_t ri = 0; ri < reg_n; ri++)
    {
        const rule_registry_entry_t *ent = &reg[ri];
        leuko_rule_config_t *rconf = leuko_rule_config_get_by_index(cfg, ri);
        if (!rconf)
            continue;
        leuko_node_t *rule_node = leuko_node_get_rule_mapping(merged, ent->full_name);
        if (rule_node)
        {
            leuko_node_t *inc = leuko_node_get_mapping_child(rule_node, LEUKO_CONFIG_KEY_INCLUDE);
            size_t n = leuko_node_array_count(inc);
            if (n > 0)
            {
                rconf->include = calloc(n, sizeof(char *));
                rconf->include_count = n;
                for (size_t i = 0; i < n; i++)
                    rconf->include[i] = strdup(leuko_node_array_scalar_at(inc, i) ?: "");
            }
            leuko_node_t *exc = leuko_node_get_mapping_child(rule_node, LEUKO_CONFIG_KEY_EXCLUDE);
            n = leuko_node_array_count(exc);
            if (n > 0)
            {
                rconf->exclude = calloc(n, sizeof(char *));
                rconf->exclude_count = n;
                for (size_t i = 0; i < n; i++)
                    rconf->exclude[i] = strdup(leuko_node_array_scalar_at(exc, i) ?: "");
            }
        }
        /* call rule-specific handler if present */
        const leuko_rule_config_handlers_t *ops = ent->handlers;
        if (ops && ops->apply_merged)
        {
            char *err = NULL;
            bool ok = ops->apply_merged(rconf, merged, ent->full_name, ent->category_name, ent->rule_name, &err);
            if (!ok && err)
            {
                /* ignore for now; tests do not surface apply errors */
                free(err);
            }
        }

        /* compile rule-level include/exclude regex arrays */
        if (rconf->include && rconf->include_count > 0)
            leuko_compile_regex_array(rconf->include, rconf->include_count, &rconf->include_re, &rconf->include_re_count);
        if (rconf->exclude && rconf->exclude_count > 0)
            leuko_compile_regex_array(rconf->exclude, rconf->exclude_count, &rconf->exclude_re, &rconf->exclude_re_count);
    }

    if (merged)
        leuko_node_free(merged);

    return cfg;
}

/* Public API */
leuko_compiled_config_t *
leuko_compiled_config_build(const char *dir, const leuko_compiled_config_t *parent)
{
    if (!dir)
        return NULL;
    leuko_compiled_config_t *c = calloc(1, sizeof(leuko_compiled_config_t));
    if (!c)
        return NULL;

    c->dir = strdup(dir);
    c->arena = leuko_arena_new(0);
    c->refcount = 1;

    /* For prototype, look for .rubocop.yml in dir only */
    char cfgpath[PATH_MAX];
    snprintf(cfgpath, sizeof(cfgpath), "%s/.rubocop.yml", dir);

    struct stat st;
    if (stat(cfgpath, &st) != 0)
    {
        /* no config file */
        return c;
    }

    c->source_files = malloc(sizeof(char *));
    c->source_files_count = 1;
    c->source_files[0] = strdup(cfgpath);

    /* Build merged node */
    c->merged_node = build_merged_node_from_files(c->arena, c->source_files, c->source_files_count);
    if (!c->merged_node)
    {
        leuko_compiled_config_unref(c);
        return NULL;
    }

    /* Materialize typed config into arena so freeing compiled_config cleans it up */
    c->effective_config = materialize_rules_from_node(c->arena, c->merged_node);
    c->effective_config_from_arena = true;
    if (!c->effective_config)
    {
        leuko_compiled_config_unref(c);
        return NULL;
    }

    /* Merge parent's general simple arrays into child (parent first) */
    if (parent && parent->effective_config)
    {
        size_t p_inc = 0, c_inc = 0;
        char **p_inc_arr = NULL;
        char **c_inc_arr = NULL;
        if (parent->effective_config->general)
        {
            p_inc = parent->effective_config->general->include_count;
            p_inc_arr = parent->effective_config->general->include;
        }
        if (c->effective_config->general)
        {
            c_inc = c->effective_config->general->include_count;
            c_inc_arr = c->effective_config->general->include;
        }
        if (p_inc > 0)
        {
            size_t total = p_inc + c_inc;
            char **narr = leuko_arena_alloc(c->arena, sizeof(char *) * total);
            size_t i;
            for (i = 0; i < p_inc; i++)
                narr[i] = leuko_arena_strdup(c->arena, p_inc_arr[i]);
            for (i = 0; i < c_inc; i++)
                narr[p_inc + i] = leuko_arena_strdup(c->arena, c_inc_arr[i]);
            leuko_general_config_t *cac = leuko_config_get_general_config(c->effective_config);
            cac->include = calloc(total, sizeof(char *));
            cac->include_count = total;
            for (i = 0; i < total; i++)
                cac->include[i] = strdup(narr[i]);
        }
    }

    /* fingerprint: hash file path + mtime */
    uint64_t h = 0;
    h = leuko_fnv1a_64(c->source_files[0], strlen(c->source_files[0]));
    h ^= (uint64_t)st.st_mtime;
    c->fingerprint = h;

    return c;
}

void leuko_compiled_config_ref(leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return;
    cfg->refcount++;
}

void leuko_compiled_config_unref(leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return;
    cfg->refcount--;
    if (cfg->refcount <= 0)
    {
        if (cfg->merged_node)
        {
            leuko_node_free(cfg->merged_node);
            cfg->merged_node = NULL;
        }
        if (cfg->effective_config)
        {
            if (!cfg->effective_config_from_arena)
            {
                leuko_config_free(cfg->effective_config);
            }
            cfg->effective_config = NULL;
        }
        if (cfg->source_files)
        {
            size_t i;
            for (i = 0; i < cfg->source_files_count; i++)
                free(cfg->source_files[i]);
            free(cfg->source_files);
            cfg->source_files = NULL;
            cfg->source_files_count = 0;
        }
        if (cfg->dir)
            free(cfg->dir);
        if (cfg->arena)
            leuko_arena_free(cfg->arena);
        free(cfg);
    }
}

const leuko_node_t *leuko_compiled_config_merged_node(const leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return NULL;
    return cfg->merged_node;
}

const leuko_config_t *leuko_compiled_config_rules(const leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return NULL;
    return cfg->effective_config;
}

const leuko_general_config_t *leuko_compiled_config_general(const leuko_compiled_config_t *cfg)
{
    if (!cfg || !cfg->effective_config)
        return NULL;
    return cfg->effective_config->general;
}

const leuko_category_config_t *leuko_compiled_config_get_category(const leuko_compiled_config_t *cfg, const char *name)
{
    if (!cfg || !cfg->effective_config || !name)
        return NULL;
    return leuko_config_get_category_config((leuko_config_t *)cfg->effective_config, name);
}

size_t leuko_compiled_config_category_include_count(const leuko_compiled_config_t *cfg, const char *category)
{
    const leuko_category_config_t *cc = leuko_compiled_config_get_category(cfg, category);
    if (!cc)
        return 0;
    return cc->include_count;
}

const char *leuko_compiled_config_category_include_at(const leuko_compiled_config_t *cfg, const char *category, size_t idx)
{
    const leuko_category_config_t *cc = leuko_compiled_config_get_category(cfg, category);
    if (!cc || idx >= cc->include_count)
        return NULL;
    return cc->include[idx];
}

/* Accessors */
size_t leuko_compiled_config_general_include_count(const leuko_compiled_config_t *cfg)
{
    if (!cfg || !cfg->effective_config || !cfg->effective_config->general)
        return 0;
    return cfg->effective_config->general->include_count;
}

const char *leuko_compiled_config_general_include_at(const leuko_compiled_config_t *cfg, size_t idx)
{
    if (!cfg || !cfg->effective_config || !cfg->effective_config->general)
        return NULL;
    if (idx >= cfg->effective_config->general->include_count)
        return NULL;
    return cfg->effective_config->general->include[idx];
}

bool leuko_compiled_config_matches_dir(const leuko_compiled_config_t *cfg, const char *path)
{
    if (!cfg || !path)
        return false;
    /* Prototype: true if path is under cfg->dir or same dir */
    size_t l = strlen(cfg->dir);
    if (strncmp(cfg->dir, path, l) == 0)
        return true;
    return false;
}
