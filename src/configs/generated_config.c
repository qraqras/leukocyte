/* Implementation of generated config init/free using RULES_LIST */
#include "configs/generated_config.h"
#include <stdlib.h>
#include <string.h>

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

void config_init_defaults(config_t *cfg)
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

void config_free(config_t *cfg)
{
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
}
