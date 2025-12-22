/*
 * Config discovery: upward walk for .rubocop.yml / rubocop.yml and merging
 */

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include "configs/discovery/discover.h"
#include "configs/discovery/raw_config.h"
#include "configs/loader/merge.h"
#include "configs/loader/yaml_helpers.h"
#include "configs/loader/loader.h"

static const char *candidates[] = {".rubocop.yml", "rubocop.yml"};
static const size_t candidates_count = 2;

/* Helper: join dir and name (alloc) */
static char *join_dir_name(const char *dir, const char *name)
{
    if (!dir || !name)
        return NULL;
    size_t dl = strlen(dir);
    size_t nl = strlen(name);
    size_t total = dl + 1 + nl + 1;
    char *p = malloc(total);
    if (!p)
        return NULL;
    memcpy(p, dir, dl);
    p[dl] = '/';
    memcpy(p + dl + 1, name, nl);
    p[total - 1] = '\0';
    return p;
}

/* Check if a yaml_document has 'root: true' at top level */
static bool doc_has_root_true(yaml_document_t *doc)
{
    if (!doc)
        return false;
    yaml_node_t *root = yaml_document_get_root_node(doc);
    if (!root || root->type != YAML_MAPPING_NODE)
        return false;
    char *v = yaml_get_mapping_scalar_value(doc, root, "root");
    if (!v)
        return false;
    if (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0 || strcasecmp(v, "yes") == 0)
        return true;
    return false;
}

int leuko_config_discover_for_file(const char *file_path, const char *cli_config_path, leuko_raw_config_t **out_raw, char **err)
{
    if (!out_raw)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }

    *out_raw = NULL;

    /* If CLI config provided, use it directly */
    if (cli_config_path && cli_config_path[0] != '\0')
    {
        leuko_raw_config_t *cfg = NULL;
        int rc = leuko_config_load_file(cli_config_path, &cfg, err);
        if (rc != 0)
            return rc;
        *out_raw = cfg;
        return 0;
    }

    /* Determine starting directory */
    char start_dir[PATH_MAX];
    if (!file_path || file_path[0] == '\0')
    {
        if (!getcwd(start_dir, sizeof(start_dir)))
        {
            if (err)
                *err = strdup("could not determine start directory");
            return 1;
        }
    }
    else
    {
        /* If file_path is a directory, use it; else use dirname(file_path) */
        struct stat st;
        if (stat(file_path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            strncpy(start_dir, file_path, sizeof(start_dir));
            start_dir[sizeof(start_dir) - 1] = '\0';
        }
        else
        {
            char *dup = strdup(file_path);
            char *d = dirname(dup);
            strncpy(start_dir, d, sizeof(start_dir));
            start_dir[sizeof(start_dir) - 1] = '\0';
            free(dup);
        }
    }

    /* Walk upward collecting candidate config docs in array (nearest last) */
    leuko_raw_config_t **found = NULL;
    size_t found_count = 0;
    char cur[PATH_MAX];
    strncpy(cur, start_dir, sizeof(cur));
    cur[sizeof(cur) - 1] = '\0';

    while (1)
    {
        for (size_t i = 0; i < candidates_count; i++)
        {
            char *candidate = join_dir_name(cur, candidates[i]);
            if (!candidate)
                continue;
            if (access(candidate, F_OK) == 0)
            {
                leuko_raw_config_t *cfg = NULL;
                char *lerr = NULL;
                int rc = leuko_config_load_file(candidate, &cfg, &lerr);
                if (rc != 0)
                {
                    if (lerr)
                    {
                        if (err)
                            *err = strdup(lerr);
                        free(lerr);
                    }
                    free(candidate);
                    /* treat load failure as fatal per policy */
                    for (size_t j = 0; j < found_count; j++)
                        leuko_raw_config_free(found[j]);
                    free(found);
                    return 1;
                }
                /* append */
                leuko_raw_config_t **tmp = realloc(found, (found_count + 1) * sizeof(leuko_raw_config_t *));
                if (!tmp)
                {
                    leuko_raw_config_free(cfg);
                    free(candidate);
                    for (size_t j = 0; j < found_count; j++)
                        leuko_raw_config_free(found[j]);
                    free(found);
                    if (err)
                        *err = strdup("allocation failure");
                    return 1;
                }
                found = tmp;
                found[found_count++] = cfg;

                /* If this config has root: true, stop upward walk */
                if (doc_has_root_true(cfg->doc))
                {
                    free(candidate);
                    goto merge_and_return;
                }
            }
            free(candidate);
        }

        /* Move up: stop if at filesystem root */
        char parent[PATH_MAX];
        strncpy(parent, cur, sizeof(parent));
        parent[sizeof(parent) - 1] = '\0';
        char *dup = strdup(parent);
        char *pdir = dirname(dup);
        if (!pdir || strcmp(pdir, cur) == 0)
        {
            free(dup);
            break;
        }
        strncpy(cur, pdir, sizeof(cur));
        cur[sizeof(cur) - 1] = '\0';
        free(dup);
    }

merge_and_return:
    if (found_count == 0)
    {
        *out_raw = NULL;
        return 0;
    }

    /* Build array of yaml_document_t* in parent-first order (reverse found) */
    size_t doc_count = found_count;
    yaml_document_t **docs = calloc(doc_count, sizeof(yaml_document_t *));
    if (!docs)
    {
        for (size_t j = 0; j < found_count; j++)
            leuko_raw_config_free(found[j]);
        free(found);
        if (err)
            *err = strdup("allocation failure");
        return 1;
    }
    for (size_t i = 0; i < doc_count; i++)
    {
        docs[i] = found[doc_count - 1 - i]->doc; /* reverse: parent-first */
    }

    yaml_document_t *merged = yaml_merge_documents_multi(docs, doc_count);
    free(docs);

    if (!merged)
    {
        for (size_t j = 0; j < found_count; j++)
            leuko_raw_config_free(found[j]);
        free(found);
        if (err)
            *err = strdup("failed to merge configs");
        return 1;
    }

    /* Create a leuko_raw_config_t wrapper for merged doc; path set to nearest config (found last) */
    leuko_raw_config_t *outcfg = malloc(sizeof(leuko_raw_config_t));
    if (!outcfg)
    {
        yaml_document_delete(merged);
        free(merged);
        for (size_t j = 0; j < found_count; j++)
            leuko_raw_config_free(found[j]);
        free(found);
        if (err)
            *err = strdup("allocation failure");
        return 1;
    }
    outcfg->doc = merged;
    outcfg->path = strdup(found[found_count - 1]->path);
    outcfg->refcount = 1;

    /* cleanup originals */
    for (size_t j = 0; j < found_count; j++)
        leuko_raw_config_free(found[j]);
    free(found);

    *out_raw = outcfg;
    return 0;
}

/* ---- Simple runtime cache for per-directory config_t derived from discovery ---- */

#include <pthread.h>

typedef struct
{
    char *start_dir;   /* starting directory used for discovery */
    char *config_path; /* path to nearest config file used */
    time_t mtime;      /* mtime of config_path when cached */
    config_t cfg;      /* runtime config (initialized via apply_config) */
} cfg_cache_entry_t;

static cfg_cache_entry_t *cfg_cache = NULL;
static size_t cfg_cache_count = 0;
static size_t cfg_cache_cap = 0;
static pthread_mutex_t cfg_cache_lock = PTHREAD_MUTEX_INITIALIZER;

/* Helper to compute start_dir same as discovery above; returns 0 on success */
static int get_start_dir_for_file(const char *file_path, char *out, size_t outlen)
{
    if (!file_path || file_path[0] == '\0')
    {
        if (!getcwd(out, outlen))
            return -1;
        return 0;
    }
    struct stat st;
    if (stat(file_path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        strncpy(out, file_path, outlen);
        out[outlen - 1] = '\0';
        return 0;
    }
    char *dup = strdup(file_path);
    if (!dup)
        return -1;
    char *d = dirname(dup);
    strncpy(out, d, outlen);
    out[outlen - 1] = '\0';
    free(dup);
    return 0;
}

int leuko_config_get_cached_config_for_file(const char *file_path, const config_t **out_cfg, char **err)
{
    if (!out_cfg)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }
    *out_cfg = NULL;

    char start_dir[PATH_MAX];
    if (get_start_dir_for_file(file_path, start_dir, sizeof(start_dir)) != 0)
    {
        if (err)
            *err = strdup("could not determine start directory");
        return 1;
    }

    /* Fast path: check cache under lock */
    pthread_mutex_lock(&cfg_cache_lock);
    for (size_t i = 0; i < cfg_cache_count; i++)
    {
        if (strcmp(cfg_cache[i].start_dir, start_dir) == 0)
        {
            /* validate mtime of config_path */
            struct stat st;
            if (stat(cfg_cache[i].config_path, &st) == 0 && st.st_mtime == cfg_cache[i].mtime)
            {
                /* cache hit */
                *out_cfg = &cfg_cache[i].cfg;
                pthread_mutex_unlock(&cfg_cache_lock);
                return 0;
            }
            else
            {
                /* invalidate this entry */
                /* fprintf(stderr, "[leuko-debug] invalidating cache for %s\n", start_dir); */
                free(cfg_cache[i].start_dir);
                free(cfg_cache[i].config_path);
                free_config(&cfg_cache[i].cfg);
                /* shift remaining entries down */
                for (size_t j = i + 1; j < cfg_cache_count; j++)
                    cfg_cache[j - 1] = cfg_cache[j];
                cfg_cache_count--;
                break;
            }
        }
    }
    pthread_mutex_unlock(&cfg_cache_lock);

    /* Not found in cache: perform discovery */
    leuko_raw_config_t *raw = NULL;
    char *lerr = NULL;
    int rc = leuko_config_discover_for_file(file_path, NULL, &raw, &lerr);
    /* debug: discovery result: rc=%d raw=%p lerr=%p */
    (void)rc;
    (void)raw;
    (void)lerr;
    if (rc != 0)
    {
        if (lerr)
        {
            if (err)
                *err = strdup(lerr);
            free(lerr);
        }
        else
        {
            if (err)
                *err = strdup("discovery failed");
        }
        return rc;
    }

    if (!raw)
    {
        /* no config discovered for this file */
        return 0;
    }

    /* Stat the nearest config file for mtime */
    struct stat st;
    if (stat(raw->path, &st) != 0)
    {
        /* treat as error */
        leuko_raw_config_free(raw);
        if (err)
            *err = strdup("could not stat discovered config");
        return 1;
    }
    /* debug: stat ok */
    (void)st;

    /* Convert merged YAML document to runtime config_t */
    config_t tmp = {0};
    initialize_config(&tmp);
    if (!apply_config(raw->doc, &tmp, &lerr))
    {
        /* debug: apply_config failed */
        leuko_raw_config_free(raw);
        free_config(&tmp);
        if (lerr)
        {
            if (err)
                *err = strdup(lerr);
            free(lerr);
        }
        else
        {
            if (err)
                *err = strdup("apply_config failed");
        }
        return 1;
    }
    /* apply_config succeeded */

    /* Insert into cache under lock; but first re-check another thread didn't insert it */
    pthread_mutex_lock(&cfg_cache_lock);
    for (size_t i = 0; i < cfg_cache_count; i++)
    {
        if (strcmp(cfg_cache[i].start_dir, start_dir) == 0)
        {
            /* Use existing entry and discard temporary */
            *out_cfg = &cfg_cache[i].cfg;
            pthread_mutex_unlock(&cfg_cache_lock);
            leuko_raw_config_free(raw);
            free_config(&tmp);
            return 0;
        }
    }

    if (cfg_cache_count == cfg_cache_cap)
    {
        size_t newcap = cfg_cache_cap == 0 ? 8 : cfg_cache_cap * 2;
        cfg_cache_entry_t *n = realloc(cfg_cache, newcap * sizeof(cfg_cache_entry_t));
        if (!n)
        {
            pthread_mutex_unlock(&cfg_cache_lock);
            leuko_raw_config_free(raw);
            free_config(&tmp);
            if (err)
                *err = strdup("allocation failure");
            return 1;
        }
        cfg_cache = n;
        cfg_cache_cap = newcap;
    }

    cfg_cache_entry_t *entry = &cfg_cache[cfg_cache_count];
    entry->start_dir = strdup(start_dir);
    entry->config_path = strdup(raw->path);
    entry->mtime = st.st_mtime;
    entry->cfg = tmp; /* take ownership of tmp */
    cfg_cache_count++;
    *out_cfg = &entry->cfg;
    pthread_mutex_unlock(&cfg_cache_lock);

    leuko_raw_config_free(raw);
    return 0;
}

/* Read-only cache lookup for worker threads. Does not perform discovery or insert.
 * Returns 0 on success; on success *out_cfg is set to the cached config pointer or NULL if not found.
 * Returns non-zero on invalid arguments. */
int leuko_config_get_cached_config_for_file_ro(const char *file_path, const config_t **out_cfg)
{
    if (!out_cfg)
        return 1;
    *out_cfg = NULL;

    char start_dir[PATH_MAX];
    if (get_start_dir_for_file(file_path, start_dir, sizeof(start_dir)) != 0)
        return 1;

    pthread_mutex_lock(&cfg_cache_lock);
    for (size_t i = 0; i < cfg_cache_count; i++)
    {
        if (strcmp(cfg_cache[i].start_dir, start_dir) == 0)
        {
            /* validate mtime of config_path without modifying cache; if mismatch treat as miss */
            struct stat st;
            if (stat(cfg_cache[i].config_path, &st) == 0 && st.st_mtime == cfg_cache[i].mtime)
            {
                *out_cfg = &cfg_cache[i].cfg;
            }
            break;
        }
    }
    pthread_mutex_unlock(&cfg_cache_lock);
    return 0;
}

void leuko_config_clear_cache(void)
{
    pthread_mutex_lock(&cfg_cache_lock);
    for (size_t i = 0; i < cfg_cache_count; i++)
    {
        free(cfg_cache[i].start_dir);
        free(cfg_cache[i].config_path);
        free_config(&cfg_cache[i].cfg);
    }
    free(cfg_cache);
    cfg_cache = NULL;
    cfg_cache_count = cfg_cache_cap = 0;
    pthread_mutex_unlock(&cfg_cache_lock);
}

/* Worker used by warm routine. Implemented at file scope because nested
 * functions are not allowed in C. */
static void *leuko_config_warm_worker(void *v)
{
    typedef struct
    {
        char **dirs;
        size_t dirs_count;
        size_t *next_idx; /* shared index */
        pthread_mutex_t *idx_lock;
        char **first_err; /* single slot to record first error */
        pthread_mutex_t *err_lock;
    } warm_ctx_t;

    warm_ctx_t *w = (warm_ctx_t *)v;
    while (1)
    {
        pthread_mutex_lock(w->idx_lock);
        size_t i = *(w->next_idx);
        if (i >= w->dirs_count)
        {
            pthread_mutex_unlock(w->idx_lock);
            break;
        }
        (*(w->next_idx))++;
        pthread_mutex_unlock(w->idx_lock);

        char *d = w->dirs[i];
        const config_t *cfg = NULL;
        char *lerr = NULL;
        int rc = leuko_config_get_cached_config_for_file(d, &cfg, &lerr);
        if (rc != 0 && lerr)
        {
            pthread_mutex_lock(w->err_lock);
            if (*(w->first_err) == NULL)
                *(w->first_err) = strdup(lerr);
            pthread_mutex_unlock(w->err_lock);
            free(lerr);
        }
        /* else success or no config: nothing to do */
    }
    return NULL;
}

/* Warm runtime config cache for a list of files. This will compute the set
 * of unique start directories and discover/apply configs for each. It uses
 * 'workers_count' threads to parallelize work; if 0 it runs single-threaded.
 */
int leuko_config_warm_cache_for_files(char **files, size_t files_count, size_t workers_count, char **err)
{
    if (!files || files_count == 0)
    {
        return 0; /* nothing to do */
    }

    /* Build unique list of start_dirs */
    char **dirs = NULL;
    size_t dirs_count = 0;
    size_t dirs_cap = 0;

    for (size_t i = 0; i < files_count; i++)
    {
        char sd[PATH_MAX];
        if (get_start_dir_for_file(files[i], sd, sizeof(sd)) != 0)
        {
            if (err)
                *err = strdup("could not determine start directory");
            /* cleanup */
            for (size_t j = 0; j < dirs_count; j++)
                free(dirs[j]);
            free(dirs);
            return 1;
        }
        bool found = false;
        for (size_t j = 0; j < dirs_count; j++)
        {
            if (strcmp(dirs[j], sd) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            if (dirs_count == dirs_cap)
            {
                size_t newcap = dirs_cap == 0 ? 16 : dirs_cap * 2;
                char **n = realloc(dirs, newcap * sizeof(char *));
                if (!n)
                {
                    if (err)
                        *err = strdup("allocation failure");
                    for (size_t j = 0; j < dirs_count; j++)
                        free(dirs[j]);
                    free(dirs);
                    return 1;
                }
                dirs = n;
                dirs_cap = newcap;
            }
            dirs[dirs_count++] = strdup(sd);
        }
    }

    if (dirs_count == 0)
    {
        free(dirs);
        return 0;
    }

    if (workers_count == 0)
        workers_count = 1;

    /* Shared state */
    size_t next_idx = 0;
    pthread_mutex_t idx_lock = PTHREAD_MUTEX_INITIALIZER;
    char *first_err = NULL;
    pthread_mutex_t err_lock = PTHREAD_MUTEX_INITIALIZER;

    typedef struct
    {
        char **dirs;
        size_t dirs_count;
        size_t *next_idx; /* shared index */
        pthread_mutex_t *idx_lock;
        char **first_err; /* single slot to record first error */
        pthread_mutex_t *err_lock;
    } warm_ctx_t;

    warm_ctx_t ctx = {.dirs = dirs, .dirs_count = dirs_count, .next_idx = &next_idx, .idx_lock = &idx_lock, .first_err = &first_err, .err_lock = &err_lock};

    /* Spawn workers */
    pthread_t *ths = calloc(workers_count, sizeof(pthread_t));
    if (!ths)
    {
        if (err)
            *err = strdup("allocation failure");
        for (size_t j = 0; j < dirs_count; j++)
            free(dirs[j]);
        free(dirs);
        return 1;
    }

    for (size_t i = 0; i < workers_count; i++)
    {
        pthread_create(&ths[i], NULL, leuko_config_warm_worker, &ctx);
    }

    for (size_t i = 0; i < workers_count; i++)
        pthread_join(ths[i], NULL);

    /* cleanup */
    free(ths);
    for (size_t j = 0; j < dirs_count; j++)
        free(dirs[j]);
    free(dirs);

    if (first_err)
    {
        if (err)
            *err = strdup(first_err);
        free(first_err);
        return 1;
    }
    return 0;
}
