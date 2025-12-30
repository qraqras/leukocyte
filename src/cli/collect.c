/* src/cli/collect.c
 * Collect Ruby files from CLI paths using resolved RuboCop config excludes
 */

#include "cli/collect.h"
#include "utils/string_array.h"
#include "cli/exit_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <unistd.h>
#include "cJSON.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static bool is_ruby_basename(const char *bn)
{
    if (!bn)
        return false;
    /* common names */
    if (strcmp(bn, "Rakefile") == 0 || strcmp(bn, "Gemfile") == 0)
        return true;
    size_t len = strlen(bn);
    if (len >= 3 && strcmp(bn + len - 3, ".rb") == 0)
        return true;
    if (len >= 6 && strcmp(bn + len - 6, ".gemspec") == 0)
        return true;
    if (len >= 5 && strcmp(bn + len - 5, ".rake") == 0)
        return true;
    return false;
}

static bool has_ruby_shebang(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    char buf[256];
    if (!fgets(buf, sizeof(buf), f))
    {
        fclose(f);
        return false;
    }
    fclose(f);
    /* check for shebang containing 'ruby' */
    if (buf[0] == '#' && buf[1] == '!')
    {
        if (strstr(buf, "ruby") != NULL)
            return true;
    }
    return false;
}

/* Normalize path to use forward slashes and be relative to cwd when possible */
static void normalize_path_for_match(const char *cwd, const char *abs_path, char *out, size_t out_sz)
{
    if (!cwd || !abs_path || !out)
        return;
    size_t cwd_len = strlen(cwd);
    if (strncmp(abs_path, cwd, cwd_len) == 0 && abs_path[cwd_len] == '/')
    {
        /* make relative with leading ./ */
        int n = snprintf(out, out_sz, ".%s", abs_path + cwd_len);
        if (n < 0)
            out[0] = '\0';
    }
    else
    {
        /* fallback to absolute but normalize slashes */
        strncpy(out, abs_path, out_sz - 1);
        out[out_sz - 1] = '\0';
    }
    /* convert backslashes to slashes (defensive) */
    for (char *p = out; *p; ++p)
        if (*p == '\\')
            *p = '/';
}

/* Check whether path (absolute) matches any pattern in a list (patterns
 * are applied to path normalized relative to cwd when possible)
 */
static bool matches_any_pattern(const char *cwd, const char *abs_path, char **patterns, size_t count)
{
    if (!patterns || count == 0)
        return false;
    char norm[PATH_MAX];
    normalize_path_for_match(cwd, abs_path, norm, sizeof(norm));
    for (size_t i = 0; i < count; ++i)
    {
        const char *pat = patterns[i];
        if (!pat)
            continue;
        int m = fnmatch(pat, norm, 0);
        if (m == 0)
            return true;
    }
    return false;
}

/* Recursively traverse directory and add ruby files that aren't excluded */
static bool traverse_dir(const char *cwd, const char *dirpath, char ***out_files, size_t *out_count, leuko_config_cache_t *cache)
{
    DIR *d = opendir(dirpath);
    if (!d)
    {
        fprintf(stderr, "Warning: could not open directory %s\n", dirpath);
        return true; /* non-fatal */
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        char child[PATH_MAX];
        int n = snprintf(child, sizeof(child), "%s/%s", dirpath, ent->d_name);
        if (n < 0 || (size_t)n >= sizeof(child))
            continue;
        struct stat st;
        if (lstat(child, &st) != 0)
        {
            fprintf(stderr, "Warning: could not stat %s\n", child);
            continue;
        }
        if (S_ISDIR(st.st_mode))
        {
            /* Directory: check exclude pattern on dir (match against path with trailing /?) */
            leuko_resolved_config_t *cfg = leuko_config_cache_find_for_path(cache, child);
            if (cfg && matches_any_pattern(cwd, child, cfg->excludes, cfg->exclude_count))
                continue;
            if (!traverse_dir(cwd, child, out_files, out_count, cache))
            {
                closedir(d);
                return false;
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            /* File: check ruby and exclude/include */
            if (!is_ruby_basename(ent->d_name))
            {
                if (!has_ruby_shebang(child))
                    continue;
            }
            leuko_resolved_config_t *cfg = leuko_config_cache_find_for_path(cache, child);
            if (cfg)
            {
                if (matches_any_pattern(cwd, child, cfg->excludes, cfg->exclude_count))
                    continue;
                if (cfg->include_count > 0 && !matches_any_pattern(cwd, child, cfg->includes, cfg->include_count))
                    continue;
            }
            if (!leuko_str_arr_push(out_files, out_count, child))
            {
                closedir(d);
                return false;
            }
        }
    }
    closedir(d);
    return true;
}

/* Parse resolved JSON to collect exclude patterns into an array (caller frees) */
static bool parse_resolved_excludes(const char *resolved_json_path, char ***out_excl, size_t *out_count)
{
    *out_excl = NULL;
    *out_count = 0;

    FILE *f = fopen(resolved_json_path, "r");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf)
    {
        fclose(f);
        return false;
    }
    if (fread(buf, 1, sz, f) != (size_t)sz)
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

    /* general.exclude */
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
                {
                    leuko_str_arr_push(out_excl, out_count, it->valuestring);
                }
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

    cJSON_Delete(root);
    free(buf);
    return true;
}

bool leuko_collect_ruby_files(const char *const *paths, size_t paths_count,
                              char ***out_files, size_t *out_count,
                              leuko_config_cache_t *cache)
{
    if (!out_files || !out_count)
        return false;
    *out_files = NULL;
    *out_count = 0;

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
        return false;

    /* If cache is NULL, behavior defaults to no includes/excludes */

    /* If no paths provided, default to current directory */
    if (!paths || paths_count == 0)
    {
        const char *single = ".";
        paths = &single;
        paths_count = 1;
    }

    for (size_t i = 0; i < paths_count; ++i)
    {
        const char *p = paths[i];
        if (!p)
            continue;
        /* Expand simple glob *? not implemented; assume explicit paths relative to cwd */
        char abs[PATH_MAX];
        if (p[0] == '/')
            strncpy(abs, p, sizeof(abs) - 1);
        else
            snprintf(abs, sizeof(abs), "%s/%s", cwd, p);
        abs[sizeof(abs) - 1] = '\0';

        struct stat st;
        if (lstat(abs, &st) != 0)
        {
            fprintf(stderr, "Warning: could not stat %s\n", abs);
            continue;
        }
        if (S_ISDIR(st.st_mode))
        {
            if (!traverse_dir(cwd, abs, out_files, out_count, cache))
                goto error;
        }
        else if (S_ISREG(st.st_mode))
        {
            char *bn = strrchr(abs, '/');
            const char *basename = bn ? bn + 1 : abs;
            if (!is_ruby_basename(basename) && !has_ruby_shebang(abs))
                continue;
            leuko_resolved_config_t *cfg = leuko_config_cache_find_for_path(cache, abs);
            if (cfg)
            {
                if (matches_any_pattern(cwd, abs, cfg->excludes, cfg->exclude_count))
                    continue;
                if (cfg->include_count > 0 && !matches_any_pattern(cwd, abs, cfg->includes, cfg->include_count))
                    continue;
            }
            if (!leuko_str_arr_push(out_files, out_count, abs))
                goto error;
        }
    }

    /* dedupe & sort (simple): use qsort then unique */
    if (*out_count > 1)
    {
        qsort(*out_files, *out_count, sizeof(char *), (int (*)(const void *, const void *))strcmp);
        size_t dst = 1;
        for (size_t i = 1; i < *out_count; ++i)
        {
            if (strcmp((*out_files)[i], (*out_files)[dst - 1]) != 0)
            {
                (*out_files)[dst++] = (*out_files)[i];
            }
            else
            {
                free((*out_files)[i]); /* duplicate */
            }
        }
        *out_count = dst;
        char **tmp = realloc(*out_files, (*out_count) * sizeof(char *));
        if (tmp)
            *out_files = tmp;
    }

    return true;

error:
    if (*out_files)
    {
        for (size_t i = 0; i < *out_count; ++i)
            free((*out_files)[i]);
        free(*out_files);
        *out_files = NULL;
        *out_count = 0;
    }
    return false;
}
