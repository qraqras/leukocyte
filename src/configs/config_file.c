#include <stdlib.h>
#include <string.h>
#include "configs/config_file.h"
#include "configs/config_defaults.h"

file_t *file_clone(const file_t *src)
{
    if (!src)
        return NULL;
    struct file_t *dst = calloc(1, sizeof(struct file_t));
    if (!dst)
        return NULL;

    // Copy common fields and typed pointer for IndentationConsistency
    dst->IndentationConsistency_common = src->IndentationConsistency_common;
    // copy include/exclude arrays if present
    if (src->IndentationConsistency_common.include && src->IndentationConsistency_common.include_count > 0)
    {
        size_t n = src->IndentationConsistency_common.include_count;
        dst->IndentationConsistency_common.include = calloc(n, sizeof(const char *));
        if (!dst->IndentationConsistency_common.include)
        {
            file_free(dst);
            return NULL;
        }
        for (size_t i = 0; i < n; i++)
            dst->IndentationConsistency_common.include[i] = strdup(src->IndentationConsistency_common.include[i]);
        dst->IndentationConsistency_common.include_count = n;
    }
    if (src->IndentationConsistency_common.exclude && src->IndentationConsistency_common.exclude_count > 0)
    {
        size_t n = src->IndentationConsistency_common.exclude_count;
        dst->IndentationConsistency_common.exclude = calloc(n, sizeof(const char *));
        if (!dst->IndentationConsistency_common.exclude)
        {
            file_free(dst);
            return NULL;
        }
        for (size_t i = 0; i < n; i++)
            dst->IndentationConsistency_common.exclude[i] = strdup(src->IndentationConsistency_common.exclude[i]);
        dst->IndentationConsistency_common.exclude_count = n;
    }
    if (src->IndentationConsistency)
    {
        dst->IndentationConsistency = malloc(sizeof(indentation_consistency_t));
        if (!dst->IndentationConsistency)
        {
            file_free(dst);
            return NULL;
        }
        *dst->IndentationConsistency = *src->IndentationConsistency;
    }

    return dst;
}

void file_free(file_t *cfg)
{
    if (!cfg)
        return;

    if (cfg->IndentationConsistency)
    {
        free(cfg->IndentationConsistency);
        cfg->IndentationConsistency = NULL;
    }

    if (cfg->IndentationConsistency_common.include)
    {
        for (size_t i = 0; i < cfg->IndentationConsistency_common.include_count; i++)
            free((void *)cfg->IndentationConsistency_common.include[i]);
        free(cfg->IndentationConsistency_common.include);
        cfg->IndentationConsistency_common.include = NULL;
        cfg->IndentationConsistency_common.include_count = 0;
    }
    if (cfg->IndentationConsistency_common.exclude)
    {
        for (size_t i = 0; i < cfg->IndentationConsistency_common.exclude_count; i++)
            free((void *)cfg->IndentationConsistency_common.exclude[i]);
        free(cfg->IndentationConsistency_common.exclude);
        cfg->IndentationConsistency_common.exclude = NULL;
        cfg->IndentationConsistency_common.exclude_count = 0;
    }

    free(cfg);
}

bool config_file_apply_yaml(file_t *cfg, const char *yaml_path, char **err)
{
    if (!cfg || !yaml_path)
    {
        if (err)
            *err = strdup("invalid args");
        return false;
    }

    // For now, call per-rule apply functions. More rules can be added similarly.
    apply_yaml_to_IndentationConsistency(yaml_path, &cfg->IndentationConsistency_common, &cfg->IndentationConsistency);
    return true;
}
