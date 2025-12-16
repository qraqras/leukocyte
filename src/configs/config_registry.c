#include <stdlib.h>
#include <string.h>
#include "rules_list.h"
#include "rule_registry.h"
#include "configs/config.h"
#include "configs/config_ops.h"

static rule_config_t **g_configs = NULL;
static size_t g_config_count = 0;

void config_registry_init_defaults(void)
{
    if (g_configs)
        return; // already initialized

    const rule_registry_entry_t *registry = get_rule_registry();
    size_t count = get_rule_registry_count();

    g_configs = calloc(count, sizeof(rule_config_t *));
    if (!g_configs)
        return;
    g_config_count = count;

    for (size_t i = 0; i < count; i++)
    {
        const struct config_ops *ops = registry[i].ops;
        if (ops && ops->initialize)
        {
            g_configs[i] = ops->initialize();
        }
        else
        {
            g_configs[i] = NULL;
        }
    }
}

rule_config_t *config_registry_get_by_index(size_t idx)
{
    if (!g_configs || idx >= g_config_count)
        return NULL;
    return g_configs[idx];
}

size_t config_registry_count(void)
{
    return g_config_count;
}

void config_registry_free(void)
{
    if (!g_configs)
        return;

    for (size_t i = 0; i < g_config_count; i++)
    {
        rule_config_t *cfg = g_configs[i];
        if (!cfg)
            continue;

        // free include/exclude lists
        if (cfg->include)
        {
            for (size_t j = 0; j < cfg->include_count; j++)
                free(cfg->include[j]);
            free(cfg->include);
        }
        if (cfg->exclude)
        {
            for (size_t j = 0; j < cfg->exclude_count; j++)
                free(cfg->exclude[j]);
            free(cfg->exclude);
        }
        if (cfg->specific_config && cfg->specific_config_free)
            cfg->specific_config_free(cfg->specific_config);
        free(cfg);
    }
    free(g_configs);
    g_configs = NULL;
    g_config_count = 0;
}
