/*
 * Config discovery: upward walk for .rubocop.yml / rubocop.yml and merging
 */

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include "configs/discover.h"
#include "configs/raw_config.h"
#include "configs/merge.h"
#include "configs/yaml_helpers.h"

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
