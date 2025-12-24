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
        yaml_document_t *local_doc = malloc(sizeof(yaml_document_t));
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
    if (!files || count == 0)
        return NULL;
    leuko_config_t *cfg = leuko_arena_alloc(a, sizeof(leuko_config_t));
    if (!cfg)
        return NULL;
    leuko_config_initialize(cfg);

    /* Very small line-oriented parser for prototype: find 'AllCops' mapping and then
     * collect subsequent '- "pattern"' lines for Include/Exclude blocks.
     */
    size_t i;
    for (i = 0; i < count; i++)
    {
        FILE *fh = fopen(files[i], "r");
        if (!fh)
            continue;
        char line[1024];
        int in_allcops = 0;
        int in_block = 0; /* 1=Include,2=Exclude */
        size_t include_cap = 4;
        size_t exclude_cap = 4;
        size_t include_count = 0;
        size_t exclude_count = 0;
        char **includes = leuko_arena_alloc(a, sizeof(char *) * include_cap);
        char **excludes = leuko_arena_alloc(a, sizeof(char *) * exclude_cap);
        while (fgets(line, sizeof(line), fh))
        {
            /* trim */
            char *p = line;
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
                p++;
            if (*p == '#')
                continue;
            if (strncmp(p, "AllCops:", 8) == 0)
            {
                in_allcops = 1;
                in_block = 0;
                continue;
            }
            if (!in_allcops)
                continue;
            if (strncmp(p, "Include:", 8) == 0 || strncmp(p, "Includes:", 9) == 0)
            {
                in_block = 1;
                continue;
            }
            if (strncmp(p, "Exclude:", 8) == 0 || strncmp(p, "Excludes:", 9) == 0)
            {
                in_block = 2;
                continue;
            }
            if (in_block && p[0] == '-')
            {
                /* parse pattern between quotes or after - */
                char *q = strchr(p, '"');
                char buf[512];
                if (q)
                {
                    q++;
                    char *end = strchr(q, '"');
                    if (end)
                    {
                        size_t len = end - q;
                        if (len >= sizeof(buf))
                            len = sizeof(buf) - 1;
                        memcpy(buf, q, len);
                        buf[len] = '\0';
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    /* after '- ' */
                    q = p + 1;
                    while (*q && (*q == ' ' || *q == '\t'))
                        q++;
                    char *end = q;
                    while (*end && *end != '\n' && *end != '\r')
                        end++;
                    size_t len = end - q;
                    if (len >= sizeof(buf))
                        len = sizeof(buf) - 1;
                    memcpy(buf, q, len);
                    buf[len] = '\0';
                }
                if (in_block == 1)
                {
                    if (include_count >= include_cap)
                    {
                        /* grow */
                        size_t nc = include_cap * 2;
                        char **narr = leuko_arena_alloc(a, sizeof(char *) * nc);
                        memcpy(narr, includes, sizeof(char *) * include_cap);
                        includes = narr;
                        include_cap = nc;
                    }
                    includes[include_count++] = leuko_arena_strdup(a, buf);
                }
                else if (in_block == 2)
                {
                    if (exclude_count >= exclude_cap)
                    {
                        size_t nc = exclude_cap * 2;
                        char **narr = leuko_arena_alloc(a, sizeof(char *) * nc);
                        memcpy(narr, excludes, sizeof(char *) * exclude_cap);
                        excludes = narr;
                        exclude_cap = nc;
                    }
                    excludes[exclude_count++] = leuko_arena_strdup(a, buf);
                }
            }
        }
        fclose(fh);
        if (include_count > 0)
        {
            cfg->all_include = includes;
            cfg->all_include_count = include_count;
        }
        if (exclude_count > 0)
        {
            cfg->all_exclude = excludes;
            cfg->all_exclude_count = exclude_count;
        }
        /* mark that rules_config memory is arena-owned */
        cfg->all_include = cfg->all_include;
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
        c->rules_config = materialize_rules_from_files(c->arena, c->source_files, c->source_files_count);
        c->rules_config_from_arena = true;
        fprintf(stderr, "[compiled_config] materialize_rules_from_files returned %p\n", (void *)c->rules_config);
        if (!c->rules_config)
        {
            LDEBUG("failed to materialize rules_config for %s", dir);
            leuko_compiled_config_unref(c);
            return NULL;
        }

        /* Merge parent's AllCops include/exclude into child's rules_config so parent-level excludes
         * apply to deeper subtrees. We concatenate parent arrays before child's arrays.
         */
        if (parent && parent->rules_config)
        {
            /* merge include arrays */
            size_t p_inc = parent->rules_config->all_include_count;
            char **p_inc_arr = parent->rules_config->all_include;
            size_t c_inc = c->rules_config->all_include_count;
            char **c_inc_arr = c->rules_config->all_include;
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
                c->rules_config->all_include = narr;
                c->rules_config->all_include_count = total;
            }

            /* merge exclude arrays */
            size_t p_exc = parent->rules_config->all_exclude_count;
            char **p_exc_arr = parent->rules_config->all_exclude;
            size_t c_exc = c->rules_config->all_exclude_count;
            char **c_exc_arr = c->rules_config->all_exclude;
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
                c->rules_config->all_exclude = narr;
                c->rules_config->all_exclude_count = total;
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
        if (cfg->rules_config)
        {
            if (!cfg->rules_config_from_arena)
            {
                leuko_config_free(cfg->rules_config);
            }
            cfg->rules_config = NULL;
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
    return cfg->rules_config;
}

/* Accessors */
size_t
leuko_compiled_config_all_include_count(const leuko_compiled_config_t *cfg)
{
    if (!cfg)
        return 0;
    return cfg->rules_config ? cfg->rules_config->all_include_count : cfg->all_include_count;
}

const char *
leuko_compiled_config_all_include_at(const leuko_compiled_config_t *cfg, size_t idx)
{
    if (!cfg)
        return NULL;
    if (cfg->rules_config)
    {
        if (idx >= cfg->rules_config->all_include_count)
            return NULL;
        return cfg->rules_config->all_include[idx];
    }
    if (idx >= cfg->all_include_count)
        return NULL;
    return cfg->all_include[idx];
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
