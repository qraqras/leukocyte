/* Implementation of generated config init/free using RULES_LIST */
#include "configs/generated_config.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration to avoid implicit declaration when used in macros */
static void free_rule_config(rule_config_t *cfg);

/// @brief Initialize a config_t structure with default values.
/// @param cfg Pointer to the config_t structure to initialize
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

/// @brief Free a config_t structure and its contents.
/// @param cfg Pointer to the config_t structure to free
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
