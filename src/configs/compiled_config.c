/*
 * compiled_config.c
 * Build merged yaml_document_t and materialize leuko_config_t
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <yaml.h>

#include "configs/compiled_config.h"
#include "configs/common/config.h"
#include "utils/allocator/arena.h"
#include "utils/string_array.h"
#include "leuko_debug.h"
#include "sources/yaml/merge.h"
#include "common/registry/registry.h"
#include <regex.h>
#include "utils/glob_to_regex.h"

/* Forward declare document-based materializer */
static leuko_config_t *materialize_rules_from_document(struct leuko_arena *a, yaml_document_t *doc);

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

/* Build a minimal merged yaml_document_t from a list of files.
 * For prototype, simply parse each file and keep last document as merged_doc
 * (full deep merge to be added later). This is sufficient for tests that
 * check presence and conversion.
 */
static yaml_document_t *
build_merged_doc_from_files(struct leuko_arena *a, char **files, size_t count)
{
    yaml_parser_t parser;
    yaml_document_t *doc = NULL;
    size_t i;

    for (i = 0; i < count; i++)
    {
        FILE *fh = fopen(files[i], "rb");
        if (!fh)
        {
            LDEBUG("failed to open %s: %s", files[i], strerror(errno));
            return NULL;
        }
        if (!yaml_parser_initialize(&parser))
        {
            fclose(fh);
            LDEBUG("yaml: parser init failed");
            return NULL;
        }
        yaml_parser_set_input_file(&parser, fh);
        yaml_document_t *local_doc = calloc(1, sizeof(yaml_document_t));
        if (!local_doc)
        {
            yaml_parser_delete(&parser);
            fclose(fh);
            return NULL;
        }
        if (!yaml_parser_load(&parser, local_doc))
        {
            yaml_document_delete(local_doc);
            free(local_doc);
            yaml_parser_delete(&parser);
            fclose(fh);
            LDEBUG("yaml: parse failed for %s", files[i]);
            return NULL;
        }
        /* Keep the last parsed document as merged_doc (prototype behaviour) */
        if (doc)
        {
            yaml_document_delete(doc);
            free(doc);
            doc = NULL;
        }
        doc = local_doc;

        yaml_parser_delete(&parser);
        fclose(fh);
    }

    return doc;
}

/* Convert merged_doc to leuko_config_t
 * For prototype, we implement a minimal converter that extracts AllCops: Include/Exclude
 * and rules: names and enabled flag.
 */
static leuko_config_t *
materialize_rules_from_files(struct leuko_arena *a, char **files, size_t count)
{
    /* Legacy entrypoint kept for compatibility: build merged doc and forward to document-based materializer */
    if (!files || count == 0)
        return NULL;
    yaml_document_t *doc = build_merged_doc_from_files(a, files, count);
    if (!doc)
        return NULL;
    leuko_config_t *cfg = materialize_rules_from_document(a, doc);
    yaml_document_delete(doc);
    free(doc);
    return cfg;
}

/* Materialize rules from a merged yaml_document_t by invoking per-rule handlers */
static leuko_config_t *
materialize_rules_from_document(struct leuko_arena *a, yaml_document_t *doc)
{
    if (!doc)
        return NULL;
    leuko_config_t *cfg = leuko_arena_alloc(a, sizeof(leuko_config_t));
    if (!cfg)
        return NULL;
    leuko_config_initialize(cfg);

    /* Populate AllCops Include/Exclude by inspecting yaml_document_t directly (safe path)
     * Note: we avoid using the leuko_yaml_node conversion here until its crash is resolved.
     */
    yaml_node_t *root = yaml_document_get_root_node(doc);
    if (root && root->type == YAML_MAPPING_NODE)
    {
        yaml_node_t *allcops = NULL;
        size_t root_pairs = 0;
        if (root->data.mapping.pairs.start && root->data.mapping.pairs.top)
            root_pairs = (size_t)(root->data.mapping.pairs.top - root->data.mapping.pairs.start);
        for (size_t j = 0; j < root_pairs; j++)
        {
            yaml_node_pair_t p2 = root->data.mapping.pairs.start[j];
            yaml_node_t *k2 = yaml_document_get_node(doc, p2.key);
            yaml_node_t *v2 = yaml_document_get_node(doc, p2.value);
            if (!k2 || k2->type != YAML_SCALAR_NODE)
                continue;
            if (strcmp((char *)k2->data.scalar.value, "AllCops") == 0)
            {
                allcops = v2;
                break;
            }
        }
        if (allcops && allcops->type == YAML_MAPPING_NODE)
        {
            fprintf(stderr, "[compiled_config] allcops=%p type=%d pairs.top=%ld\n", (void *)allcops, allcops->type, (long)allcops->data.mapping.pairs.top);
            yaml_node_t *inc = leuko_yaml_find_mapping_value(doc, allcops, LEUKO_CONFIG_KEY_INCLUDE);
            if (inc && inc->type == YAML_SEQUENCE_NODE)
            {
                size_t n = 0;
                if (inc->data.sequence.items.start && inc->data.sequence.items.top)
                    n = (size_t)(inc->data.sequence.items.top - inc->data.sequence.items.start);
                if (n > 0)
                {
                    /* store in typed AllCops struct */
                    leuko_all_cops_config_t *acfg = leuko_config_all_cops(cfg);
                    acfg->include = calloc(n, sizeof(char *));
                    acfg->include_count = n;
                    for (size_t i = 0; i < n; i++)
                    {
                        yaml_node_t *it = yaml_document_get_node(doc, inc->data.sequence.items.start[i]);
                        const char *s = (it && it->type == YAML_SCALAR_NODE) ? (const char *)it->data.scalar.value : "";
                        acfg->include[i] = strdup(s);
                    }
                }
            }
            yaml_node_t *exc = leuko_yaml_find_mapping_value(doc, allcops, LEUKO_CONFIG_KEY_EXCLUDE);
            if (exc && exc->type == YAML_SEQUENCE_NODE)
            {
                size_t n = 0;
                if (exc->data.sequence.items.start && exc->data.sequence.items.top)
                    n = (size_t)(exc->data.sequence.items.top - exc->data.sequence.items.start);
                if (n > 0)
                {
                    /* store in typed AllCops struct */
                    leuko_all_cops_config_t *acfg = leuko_config_all_cops(cfg);
                    acfg->exclude = calloc(n, sizeof(char *));
                    acfg->exclude_count = n;
                    for (size_t i = 0; i < n; i++)
                    {
                        yaml_node_t *it = yaml_document_get_node(doc, exc->data.sequence.items.start[i]);
                        const char *s = (it && it->type == YAML_SCALAR_NODE) ? (const char *)it->data.scalar.value : "";
                        acfg->exclude[i] = strdup(s);
                    }
                }
            }

            /* compile precompiled regex patterns for AllCops (prototype) */
            leuko_all_cops_config_t *acfg = leuko_config_all_cops(cfg);
            if (acfg)
            {
                /* helper: compile include patterns */
                if (acfg->include && acfg->include_count > 0)
                {
                    acfg->include_re = NULL;
                    acfg->include_re_count = 0;
                    for (size_t i = 0; i < acfg->include_count; i++)
                    {
                        char *re_s = leuko_glob_to_regex(acfg->include[i]);
                        if (!re_s)
                            continue;
                        regex_t r;
                        int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
                        free(re_s);
                        if (rc != 0)
                            continue;
                        regex_t *narr = realloc(acfg->include_re, sizeof(regex_t) * (acfg->include_re_count + 1));
                        if (!narr)
                        {
                            regfree(&r);
                            continue;
                        }
                        acfg->include_re = narr;
                        acfg->include_re[acfg->include_re_count] = r;
                        acfg->include_re_count++;
                    }
                }

                /* helper: compile exclude patterns */
                if (acfg->exclude && acfg->exclude_count > 0)
                {
                    acfg->exclude_re = NULL;
                    acfg->exclude_re_count = 0;
                    for (size_t i = 0; i < acfg->exclude_count; i++)
                    {
                        char *re_s = leuko_glob_to_regex(acfg->exclude[i]);
                        if (!re_s)
                            continue;
                        regex_t r;
                        int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
                        free(re_s);
                        if (rc != 0)
                            continue;
                        regex_t *narr = realloc(acfg->exclude_re, sizeof(regex_t) * (acfg->exclude_re_count + 1));
                        if (!narr)
                        {
                            regfree(&r);
                            continue;
                        }
                        acfg->exclude_re = narr;
                        acfg->exclude_re[acfg->exclude_re_count] = r;
                        acfg->exclude_re_count++;
                    }
                }
            }
        }

        /* Extract per-category include/exclude (inherit_mode is used only during merging and not stored) */
        for (size_t j = 0; j < root_pairs; j++)
        {
            yaml_node_pair_t p2 = root->data.mapping.pairs.start[j];
            yaml_node_t *k2 = yaml_document_get_node(doc, p2.key);
            yaml_node_t *v2 = yaml_document_get_node(doc, p2.value);
            if (!k2 || k2->type != YAML_SCALAR_NODE || !v2 || v2->type != YAML_MAPPING_NODE)
                continue;
            const char *catname = (const char *)k2->data.scalar.value;
            if (strcmp(catname, "AllCops") == 0)
                continue;
            leuko_category_config_t *cc = leuko_config_add_category(cfg, catname);
            yaml_node_t *cinc = leuko_yaml_find_mapping_value(doc, v2, LEUKO_CONFIG_KEY_INCLUDE);
            if (cinc && cinc->type == YAML_SEQUENCE_NODE)
            {
                size_t n = 0;
                if (cinc->data.sequence.items.start && cinc->data.sequence.items.top)
                    n = (size_t)(cinc->data.sequence.items.top - cinc->data.sequence.items.start);
                if (n > 0)
                {
                    cc->include = calloc(n, sizeof(char *));
                    cc->include_count = n;
                    for (size_t i = 0; i < n; i++)
                    {
                        yaml_node_t *it = yaml_document_get_node(doc, cinc->data.sequence.items.start[i]);
                        const char *s = (it && it->type == YAML_SCALAR_NODE) ? (const char *)it->data.scalar.value : "";
                        cc->include[i] = strdup(s);
                    }

                    /* compile include patterns for category */
                    cc->include_re = NULL;
                    cc->include_re_count = 0;
                    for (size_t i = 0; i < cc->include_count; i++)
                    {
                        char *re_s = leuko_glob_to_regex(cc->include[i]);
                        if (!re_s)
                            continue;
                        regex_t r;
                        int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
                        free(re_s);
                        if (rc != 0)
                            continue;
                        regex_t *narr = realloc(cc->include_re, sizeof(regex_t) * (cc->include_re_count + 1));
                        if (!narr)
                        {
                            regfree(&r);
                            continue;
                        }
                        cc->include_re = narr;
                        cc->include_re[cc->include_re_count] = r;
                        cc->include_re_count++;
                    }
                }
            }
            yaml_node_t *cexc = leuko_yaml_find_mapping_value(doc, v2, LEUKO_CONFIG_KEY_EXCLUDE);
            if (cexc && cexc->type == YAML_SEQUENCE_NODE)
            {
                size_t n = 0;
                if (cexc->data.sequence.items.start && cexc->data.sequence.items.top)
                    n = (size_t)(cexc->data.sequence.items.top - cexc->data.sequence.items.start);
                if (n > 0)
                {
                    cc->exclude = calloc(n, sizeof(char *));
                    cc->exclude_count = n;
                    for (size_t i = 0; i < n; i++)
                    {
                        yaml_node_t *it = yaml_document_get_node(doc, cexc->data.sequence.items.start[i]);
                        const char *s = (it && it->type == YAML_SCALAR_NODE) ? (const char *)it->data.scalar.value : "";
                        cc->exclude[i] = strdup(s);
                    }

                    /* compile exclude patterns for category */
                    cc->exclude_re = NULL;
                    cc->exclude_re_count = 0;
                    for (size_t i = 0; i < cc->exclude_count; i++)
                    {
                        char *re_s = leuko_glob_to_regex(cc->exclude[i]);
                        if (!re_s)
                            continue;
                        regex_t r;
                        int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
                        free(re_s);
                        if (rc != 0)
                            continue;
                        regex_t *narr = realloc(cc->exclude_re, sizeof(regex_t) * (cc->exclude_re_count + 1));
                        if (!narr)
                        {
                            regfree(&r);
                            continue;
                        }
                        cc->exclude_re = narr;
                        cc->exclude_re[cc->exclude_re_count] = r;
                        cc->exclude_re_count++;
                    }
                }
            }
        }
    }

    /* Normalize rule keys in the merged document and (future) apply per-rule handlers.
     * For now we perform normalization so downstream code sees a consistent view where
     * rule-specific nested mappings are also visible as full-name keys (Category/Rule).
     */
    leuko_yaml_node_t *merged = leuko_yaml_node_from_document(doc);
    if (merged)
    {
        leuko_yaml_normalize_rule_keys(merged);

        /* Populate per-rule include/exclude from merged node (if present) so handlers need not do it */
        const rule_registry_entry_t *reg = leuko_get_rule_registry();
        size_t reg_n = leuko_get_rule_registry_count();
        for (size_t ri = 0; ri < reg_n; ri++)
        {
            const rule_registry_entry_t *ent = &reg[ri];
            leuko_rule_config_t *rconf = leuko_rule_config_get_by_index(cfg, ri);
            if (!rconf)
                continue;
            leuko_yaml_node_t *rule_node = leuko_yaml_node_get_rule_mapping(merged, ent->full_name);
            if (rule_node)
            {
                leuko_yaml_node_t *inc = leuko_yaml_node_get_mapping_child(rule_node, LEUKO_CONFIG_KEY_INCLUDE);
                size_t n = leuko_yaml_node_sequence_count(inc);
                if (n > 0)
                {
                    rconf->include = calloc(n, sizeof(char *));
                    rconf->include_count = n;
                    for (size_t i = 0; i < n; i++)
                    {
                        const char *s = leuko_yaml_node_sequence_scalar_at(inc, i);
                        rconf->include[i] = strdup(s ? s : "");
                    }
                }
                leuko_yaml_node_t *exc = leuko_yaml_node_get_mapping_child(rule_node, LEUKO_CONFIG_KEY_EXCLUDE);
                n = leuko_yaml_node_sequence_count(exc);
                if (n > 0)
                {
                    rconf->exclude = calloc(n, sizeof(char *));
                    rconf->exclude_count = n;
                    for (size_t i = 0; i < n; i++)
                    {
                        const char *s = leuko_yaml_node_sequence_scalar_at(exc, i);
                        rconf->exclude[i] = strdup(s ? s : "");
                    }
                }
            }
        }

        /* Call per-rule apply_merged handlers to populate cfg (use index-based access matching registry order) */
        for (size_t ri = 0; ri < reg_n; ri++)
        {
            const rule_registry_entry_t *ent = &reg[ri];
            leuko_rule_config_t *rconf = leuko_rule_config_get_by_index(cfg, ri);
            if (!rconf)
                continue;
            const leuko_rule_config_handlers_t *ops = ent->handlers;
            if (!ops || !ops->apply_merged)
                continue;
            char *err = NULL;
            bool ok = ops->apply_merged(rconf, merged, ent->full_name, ent->category_name, ent->rule_name, &err);
            if (!ok)
            {
                LDEBUG("apply_merged failed for %s: %s", ent->full_name, err ? err : "(null)");
            }
            if (err)
                free(err);
        }

        /* After per-rule handlers, precompile rule-level include/exclude patterns */
        for (size_t ri = 0; ri < reg_n; ri++)
        {
            leuko_rule_config_t *rconf = leuko_rule_config_get_by_index(cfg, ri);
            if (!rconf)
                continue;
            /* compile includes */
            if (rconf->include && rconf->include_count > 0)
            {
                rconf->include_re = NULL;
                rconf->include_re_count = 0;
                for (size_t i = 0; i < rconf->include_count; i++)
                {
                    char *re_s = leuko_glob_to_regex(rconf->include[i]);
                    if (!re_s)
                        continue;
                    regex_t r;
                    int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
                    free(re_s);
                    if (rc != 0)
                        continue;
                    regex_t *narr = realloc(rconf->include_re, sizeof(regex_t) * (rconf->include_re_count + 1));
                    if (!narr)
                    {
                        regfree(&r);
                        continue;
                    }
                    rconf->include_re = narr;
                    rconf->include_re[rconf->include_re_count] = r;
                    rconf->include_re_count++;
                }
            }
            /* compile excludes */
            if (rconf->exclude && rconf->exclude_count > 0)
            {
                rconf->exclude_re = NULL;
                rconf->exclude_re_count = 0;
                for (size_t i = 0; i < rconf->exclude_count; i++)
                {
                    char *re_s = leuko_glob_to_regex(rconf->exclude[i]);
                    if (!re_s)
                        continue;
                    regex_t r;
                    int rc = regcomp(&r, re_s, REG_EXTENDED | REG_NOSUB);
                    free(re_s);
                    if (rc != 0)
                        continue;
                    regex_t *narr = realloc(rconf->exclude_re, sizeof(regex_t) * (rconf->exclude_re_count + 1));
                    if (!narr)
                    {
                        regfree(&r);
                        continue;
                    }
                    rconf->exclude_re = narr;
                    rconf->exclude_re[rconf->exclude_re_count] = r;
                    rconf->exclude_re_count++;
                }
            }
        }

        leuko_yaml_node_free(merged);
    }

    return cfg;
}

leuko_compiled_config_t *
leuko_compiled_config_build(const char *dir, const leuko_compiled_config_t *parent)
{
    if (!dir)
        return NULL;
    leuko_compiled_config_t *c = calloc(1, sizeof(leuko_compiled_config_t));
    if (!c)
        return NULL;

    c->dir = strdup(dir);
    c->arena = leuko_arena_new(0); /* struct leuko_arena* */
    c->refcount = 1;

    /* For prototype, look for .rubocop.yml in dir only */
    char cfgpath[PATH_MAX];
    snprintf(cfgpath, sizeof(cfgpath), "%s/.rubocop.yml", dir);

    struct stat st;
    if (stat(cfgpath, &st) == 0)
    {
        /* we have a config file */
        c->source_files = malloc(sizeof(char *));
        c->source_files_count = 1;
        c->source_files[0] = strdup(cfgpath);

        /* build merged_doc (prototype: last file) */
        fprintf(stderr, "[compiled_config] parsing %s\n", c->source_files[0]);
        c->merged_doc = build_merged_doc_from_files(c->arena, c->source_files, c->source_files_count);
        fprintf(stderr, "[compiled_config] build_merged_doc returned %p\n", (void *)c->merged_doc);
        if (!c->merged_doc)
        {
            LDEBUG("failed to build merged_doc for %s", dir);
            leuko_compiled_config_unref(c);
            return NULL;
        }

        fprintf(stderr, "[compiled_config] materialize_rules_from_files start\n");
        /* Build leuko_yaml_node merged tree and materialize rules from it */
        fprintf(stderr, "[compiled_config] materialize: about to call materialize_rules_from_document, merged_doc=%p\n", (void *)c->merged_doc);
        fflush(stderr);
        c->effective_config = materialize_rules_from_document(c->arena, c->merged_doc);
        c->effective_config_from_arena = true;
        if (!c->effective_config)
        {
            LDEBUG("materialize_rules_from_document failed for %s", dir);
            leuko_compiled_config_unref(c);
            return NULL;
        }

        /* Merge parent's AllCops include/exclude into child's effective_config so parent-level excludes
         * apply to deeper subtrees. We concatenate parent arrays before child's arrays.
         */
        if (parent && parent->effective_config)
        {
            /* merge include arrays (prefer typed parent->all_cops if present, otherwise legacy arrays) */
            size_t p_inc = 0;
            char **p_inc_arr = NULL;
            if (parent->effective_config->all_cops)
            {
                p_inc = parent->effective_config->all_cops->include_count;
                p_inc_arr = parent->effective_config->all_cops->include;
            }
            else
            {
                p_inc = parent->effective_config->all_include_count;
                p_inc_arr = parent->effective_config->all_include;
            }
            size_t c_inc = 0;
            char **c_inc_arr = NULL;
            if (c->effective_config->all_cops)
            {
                c_inc = c->effective_config->all_cops->include_count;
                c_inc_arr = c->effective_config->all_cops->include;
            }
            if (p_inc > 0)
            {
                size_t total = p_inc + c_inc;
                char **narr = leuko_arena_alloc(c->arena, sizeof(char *) * total);
                size_t i;
                for (i = 0; i < p_inc; i++)
                {
                    narr[i] = leuko_arena_strdup(c->arena, p_inc_arr[i]);
                }
                for (i = 0; i < c_inc; i++)
                {
                    narr[p_inc + i] = leuko_arena_strdup(c->arena, c_inc_arr[i]);
                }
                /* also update typed AllCops include (arena allocation is fine here) */
                leuko_all_cops_config_t *cac = leuko_config_all_cops(c->effective_config);
                cac->include = calloc(total, sizeof(char *));
                cac->include_count = total;
                for (i = 0; i < total; i++)
                    cac->include[i] = strdup(narr[i]);
            }

            /* merge exclude arrays (prefer typed parent) */
            size_t p_exc = 0;
            char **p_exc_arr = NULL;
            if (parent->effective_config->all_cops)
            {
                p_exc = parent->effective_config->all_cops->exclude_count;
                p_exc_arr = parent->effective_config->all_cops->exclude;
            }
            else
            {
                p_exc = parent->effective_config->all_exclude_count;
                p_exc_arr = parent->effective_config->all_exclude;
            }
            size_t c_exc = 0;
            char **c_exc_arr = NULL;
            if (c->effective_config->all_cops)
            {
                c_exc = c->effective_config->all_cops->exclude_count;
                c_exc_arr = c->effective_config->all_cops->exclude;
            }
            if (p_exc > 0)
            {
                size_t total = p_exc + c_exc;
                char **narr = leuko_arena_alloc(c->arena, sizeof(char *) * total);
                size_t i;
                for (i = 0; i < p_exc; i++)
                {
                    narr[i] = leuko_arena_strdup(c->arena, p_exc_arr[i]);
                }
                for (i = 0; i < c_exc; i++)
                {
                    narr[p_exc + i] = leuko_arena_strdup(c->arena, c_exc_arr[i]);
                }

                leuko_all_cops_config_t *cac = leuko_config_all_cops(c->effective_config);
                cac->exclude = calloc(total, sizeof(char *));
                cac->exclude_count = total;
                for (i = 0; i < total; i++)
                    cac->exclude[i] = strdup(narr[i]);
            }
        }

        /* compute a fingerprint: here simple hash of mtime + file path */
        uint64_t h = 0;
        h = leuko_fnv1a_64(c->source_files[0], strlen(c->source_files[0]));
        h ^= (uint64_t)st.st_mtime;
        c->fingerprint = h;
    }

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
        if (cfg->merged_doc)
        {
            yaml_document_delete(cfg->merged_doc);
            free(cfg->merged_doc);
            cfg->merged_doc = NULL;
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

const yaml_document_t *
leuko_compiled_config_merged_doc(const leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return NULL;
    return cfg->merged_doc;
}

const leuko_config_t *
leuko_compiled_config_rules(const leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return NULL;
    return cfg->effective_config;
}

const leuko_all_cops_config_t *
leuko_compiled_config_all_cops(const leuko_compiled_config_t *cfg)
{
    if (!cfg || !cfg->effective_config)
        return NULL;
    return cfg->effective_config->all_cops;
}

const leuko_category_config_t *
leuko_compiled_config_get_category(const leuko_compiled_config_t *cfg, const char *name)
{
    if (!cfg || !cfg->effective_config || !name)
        return NULL;
    return leuko_config_get_category((leuko_config_t *)cfg->effective_config, name);
}

size_t
leuko_compiled_config_category_include_count(const leuko_compiled_config_t *cfg, const char *category)
{
    const leuko_category_config_t *cc = leuko_compiled_config_get_category(cfg, category);
    if (!cc)
        return 0;
    return cc->include_count;
}

const char *
leuko_compiled_config_category_include_at(const leuko_compiled_config_t *cfg, const char *category, size_t idx)
{
    const leuko_category_config_t *cc = leuko_compiled_config_get_category(cfg, category);
    if (!cc || idx >= cc->include_count)
        return NULL;
    return cc->include[idx];
}

/* Accessors */
size_t
leuko_compiled_config_all_include_count(const leuko_compiled_config_t *cfg)
{
    if (!cfg || !cfg->effective_config || !cfg->effective_config->all_cops)
        return 0;
    return cfg->effective_config->all_cops->include_count;
}

const char *
leuko_compiled_config_all_include_at(const leuko_compiled_config_t *cfg, size_t idx)
{
    if (!cfg || !cfg->effective_config || !cfg->effective_config->all_cops)
        return NULL;
    if (idx >= cfg->effective_config->all_cops->include_count)
        return NULL;
    return cfg->effective_config->all_cops->include[idx];
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
