#include <stdlib.h>
#include <string.h>
#include "configs/generated_config.h"
#include "rule_registry.h"

/// @brief Initialize a config_t structure.
/// @param cfg Pointer to the config_t structure to initialize
void initialize_config(config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));

#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
    do                                               \
    {                                                \
        if (ops_ptr && (ops_ptr)->initialize)        \
        {                                            \
            cfg->field = (ops_ptr)->initialize();    \
        }                                            \
        else                                         \
        {                                            \
            cfg->field = NULL;                       \
        }                                            \
    } while (0);
    RULES_LIST
#undef X
}

/// @brief Get a pointer to a rule_config_t field by its index.
/// @param cfg Pointer to the config_t structure
/// @param idx Index of the rule_config_t field
/// @return Pointer to the rule_config_t field, or NULL if not found
rule_config_t *get_rule_config_by_index(config_t *cfg, size_t idx)
{
    if (!cfg)
    {
        return NULL;
    }
    size_t cur = 0;

#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
    do                                               \
    {                                                \
        if (cur == idx)                              \
        {                                            \
            return cfg->field;                       \
        }                                            \
        cur++;                                       \
    } while (0);
    RULES_LIST
#undef X

    return NULL;
}

/// @brief Get the total number of rule_config_t fields in config_t.
/// @return Total number of rule_config_t fields
size_t config_count(void)
{
    return get_rule_registry_count();
}

/// @brief Free a rule_config_t structure.
/// @param cfg Pointer to the rule_config_t structure to free
static void free_rule_config(rule_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    if (cfg->include)
    {
        for (size_t i = 0; i < cfg->include_count; i++)
        {
            free(cfg->include[i]);
        }
        free(cfg->include);
    }
    if (cfg->exclude)
    {
        for (size_t i = 0; i < cfg->exclude_count; i++)
        {
            free(cfg->exclude[i]);
        }
        free(cfg->exclude);
    }
    if (cfg->specific_config && cfg->specific_config_free)
    {
        cfg->specific_config_free(cfg->specific_config);
    }
    free(cfg);
}

/// @brief Free a config_t structure.
/// @param cfg Pointer to the config_t structure to free
void free_config(config_t *cfg)
{
    if (!cfg)
    {
        return;
    }

#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
    do                                               \
    {                                                \
        free_rule_config(cfg->field);                \
        cfg->field = NULL;                           \
    } while (0);
    RULES_LIST
#undef X
}
