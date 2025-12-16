/* Implementation of generated config init/free using RULES_LIST */
#include "configs/generated_config.h"
#include <stdlib.h>
#include <string.h>
#include "rule_registry.h"

/* Forward declaration to avoid implicit declaration when used in macros */
static void free_rule_config(rule_config_t *cfg);

/* Global registry instance and count (compatibility with old config_registry)
 * The functions below provide the same API as the old config_registry module
 * but are implemented on top of the generated config structure.
 */
/* Global registry instance and accessors. When callers want a global
 * registry-backed config, they call `config_initialize(NULL)` which will
 * allocate and initialize the global instance. Passing NULL to `config_free`
 * frees that global instance.
 */
static config_t *g_config = NULL;
static size_t g_config_count = 0;

/* Internal initializer used for both heap-allocated global and caller-owned configs */
static void config_initialize_internal(config_t *cfg)
{
    if (!cfg)
        return;
    memset(cfg, 0, sizeof(*cfg));

#define X(field, cat, name, rule_ptr, ops_ptr)    \
    do                                            \
    {                                             \
        if (ops_ptr && (ops_ptr)->initialize)     \
            cfg->field = (ops_ptr)->initialize(); \
        else                                      \
            cfg->field = NULL;                    \
    } while (0);
    RULES_LIST
#undef X
}

void config_initialize(config_t *cfg)
{
    if (cfg == NULL)
    {
        if (g_config)
            return; /* already initialized */
        size_t count = get_rule_registry_count();
        g_config = malloc(sizeof(*g_config));
        if (!g_config)
            return;
        config_initialize_internal(g_config);
        g_config_count = count;
    }
    else
    {
        config_initialize_internal(cfg);
    }
}

void config_free(config_t *cfg)
{
    if (cfg == NULL)
    {
        if (!g_config)
            return;
        /* free global */
        config_t *to_free = g_config;
        g_config = NULL;
        g_config_count = 0;
        /* fall through to free contents */
        cfg = to_free;
    }

    if (!cfg)
        return;

#define X(field, cat, name, rule_ptr, ops_ptr) \
    do                                         \
    {                                          \
        free_rule_config(cfg->field);          \
        cfg->field = NULL;                     \
    } while (0);
    RULES_LIST
#undef X

    /* do not free the config struct itself here for caller-owned configs */
}

rule_config_t *config_get_by_index(size_t idx)
{
    if (!g_config || idx >= g_config_count)
        return NULL;
    size_t cur = 0;

#define X(field, cat, name, rule_ptr, ops_ptr) \
    do                                         \
    {                                          \
        if (cur == idx)                        \
            return g_config->field;            \
        cur++;                                 \
    } while (0);
    RULES_LIST
#undef X

    return NULL;
}

size_t config_count(void)
{
    return g_config_count;
}

/// @brief Free a rule_config_t structure.
/// @param cfg Pointer to the rule_config_t structure to free
static void free_rule_config(rule_config_t *cfg)
{
    if (!cfg)
        return;
    if (cfg->include)
    {
        for (size_t i = 0; i < cfg->include_count; i++)
            free(cfg->include[i]);
        free(cfg->include);
    }
    if (cfg->exclude)
    {
        for (size_t i = 0; i < cfg->exclude_count; i++)
            free(cfg->exclude[i]);
        free(cfg->exclude);
    }
    if (cfg->specific_config && cfg->specific_config_free)
        cfg->specific_config_free(cfg->specific_config);
    free(cfg);
}
