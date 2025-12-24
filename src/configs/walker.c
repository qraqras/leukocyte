/*
 * walker.c
 * Directory walker that builds compiled_config per-directory and collects Ruby files
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fnmatch.h>

#include "configs/walker.h"
#include "configs/compiled_config.h"
#include "configs/common/config.h" /* full config structure */
#include <limits.h>
#include "leuko_debug.h"

static bool is_ruby_file(const char *name)
{
    size_t n = strlen(name);
    if (n >= 3 && strcmp(name + n - 3, ".rb") == 0)
        return true;
    return false;
}

static bool matches_any_pattern(const char *path, char **patterns, size_t count)
{
    if (!patterns || count == 0)
        return false;
    for (size_t i = 0; i < count; i++)
    {
        if (!patterns[i])
            continue;
        if (fnmatch(patterns[i], path, 0) == 0)
            return true;
    }
    return false;
}

/* Permissive directory match: try full path and basename; also allow patterns
 * with leading token (e.g. vendor/**) to match a directory named 'vendor'.
 */
static bool dir_matches_pattern(const char *path, const char *basename, char **patterns, size_t count)
{
    if (!patterns || count == 0)
        return false;
    for (size_t i = 0; i < count; i++)
    {
        const char *p = patterns[i];
        if (!p)
            continue;
        if (fnmatch(p, path, 0) == 0)
            return true;
        if (fnmatch(p, basename, 0) == 0)
            return true;
        const char *slash = strchr(p, '/');
        if (slash)
        {
            size_t token_len = slash - p;
            if (strncmp(p, basename, token_len) == 0 && basename[token_len] == '\0')
                return true;
        }
    }
    return false;
}

/* Helper: push file to dynamic array */
static bool push_file(leuko_collected_file_t **files, size_t *count, const char *path, leuko_compiled_config_t *cfg)
{
    leuko_collected_file_t *n = realloc(*files, sizeof(leuko_collected_file_t) * ((*count) + 1));
    if (!n)
        return false;
    *files = n;
    (*files)[*count].path = strdup(path);
    (*files)[*count].config = cfg;
    leuko_compiled_config_ref(cfg);
    (*count)++;
    return true;
}

static int walk_dir(const char *dir, leuko_compiled_config_t *parent_cfg, leuko_collected_file_t **files, size_t *count)
{
    struct stat st;
    if (stat(dir, &st) != 0)
        return -1;

    /* Build compiled_config for this dir (prototype: reads local .rubocop.yml if present) */
    leuko_compiled_config_t *cfg = leuko_compiled_config_build(dir, parent_cfg);

    /* Determine pruning by checking AllCops excludes against this directory path */
    if (cfg)
    {
        /* If configured to exclude this directory itself, stop descending */
        if (matches_any_pattern(dir, cfg->rules_config ? cfg->rules_config->all_exclude : cfg->all_exclude, cfg->rules_config ? cfg->rules_config->all_exclude_count : cfg->all_exclude_count))
        {
            leuko_compiled_config_unref(cfg);
            return 0; /* prune */
        }
    }

    DIR *d = opendir(dir);
    if (!d)
    {
        if (cfg)
            leuko_compiled_config_unref(cfg);
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, name);

        if (ent->d_type == DT_DIR)
        {
            /* Descend: but first check if current cfg excludes this child subtree */
            const char *basename = name;
            if (cfg)
            {
                if (dir_matches_pattern(path, basename, cfg->rules_config ? cfg->rules_config->all_exclude : cfg->all_exclude,
                                        cfg->rules_config ? cfg->rules_config->all_exclude_count : cfg->all_exclude_count))
                {
                    /* prune */
                    continue;
                }
            }
            walk_dir(path, cfg, files, count);
        }
        else if (ent->d_type == DT_REG)
        {
            /* Candidate file */
            if (!is_ruby_file(name))
                continue;
            /* Global AllCops include/exclude */
            bool excluded = false;
            bool included = true;
            if (cfg)
            {
                /* include semantics: if include list exists, must match one */
                size_t all_inc_count = cfg->rules_config ? cfg->rules_config->all_include_count : cfg->all_include_count;
                char **all_inc = cfg->rules_config ? cfg->rules_config->all_include : cfg->all_include;
                if (all_inc_count > 0)
                {
                    included = matches_any_pattern(path, all_inc, all_inc_count);
                }
                size_t all_exc_count = cfg->rules_config ? cfg->rules_config->all_exclude_count : cfg->all_exclude_count;
                char **all_exc = cfg->rules_config ? cfg->rules_config->all_exclude : cfg->all_exclude;
                if (all_exc_count > 0 && matches_any_pattern(path, all_exc, all_exc_count))
                    excluded = true;
            }
            if (included && !excluded)
            {
                if (!cfg)
                {
                    /* build parentless config for dir if none */
                    cfg = leuko_compiled_config_build(dir, parent_cfg);
                    if (!cfg)
                        continue;
                }
                push_file(files, count, path, cfg);
            }
        }
    }

    closedir(d);
    if (cfg)
        leuko_compiled_config_unref(cfg);
    return 0;
}

int leuko_config_walker_collect(const char *start_dir, leuko_collected_file_t **out_files, size_t *out_count)
{
    if (!start_dir || !out_files || !out_count)
        return -1;
    *out_files = NULL;
    *out_count = 0;

    int r = walk_dir(start_dir, NULL, out_files, out_count);
    if (r != 0)
    {
        /* free any pushed files */
        leuko_collected_files_free(*out_files, *out_count);
        *out_files = NULL;
        *out_count = 0;
        return -1;
    }
    return 0;
}

void leuko_collected_files_free(leuko_collected_file_t *files, size_t count)
{
    if (!files)
        return;
    for (size_t i = 0; i < count; i++)
    {
        free(files[i].path);
        leuko_compiled_config_unref(files[i].config);
    }
    free(files);
}
