#include <stdlib.h>
#include <string.h>

#include "configs/config.h"
#include "rule_registry.h"

/**
 * @brief Initialize a leuko_config_t structure.
 * @param cfg Pointer to the leuko_config_t structure to initialize
 */
void leuko_config_initialize(leuko_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));

    /* Initialize AllCops fields */
    cfg->all_include = NULL;
    cfg->all_include_count = 0;
    cfg->all_exclude = NULL;
    cfg->all_exclude_count = 0;

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
    LEUKO_RULES_LIST
#undef X
}

/**
 * @brief Get a pointer to a leuko_rule_config_t field by its index.
 * @param cfg Pointer to the leuko_config_t structure
 * @param idx Index of the leuko_rule_config_t field
 * @return Pointer to the leuko_rule_config_t field, or NULL if not found
 */
leuko_rule_config_t *leuko_rule_config_get_by_index(leuko_config_t *cfg, size_t idx)
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
    LEUKO_RULES_LIST
#undef X

    return NULL;
}

/**
 * @brief Get the total number of leuko_rule_config_t fields in leuko_config_t.
 * @return Total number of leuko_rule_config_t fields
 */
size_t leuko_config_count(void)
{
    return leuko_get_rule_registry_count();
}

/**
 * @brief Free a leuko_rule_config_t structure.
 * @param cfg Pointer to the leuko_rule_config_t structure to free
 */
static void leuko_rule_config_free(leuko_rule_config_t *cfg)
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

/**
 * @brief Free a leuko_config_t structure.
 * @param cfg Pointer to the leuko_config_t structure to free
 */
void leuko_config_free(leuko_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }

#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
    do                                               \
    {                                                \
        leuko_rule_config_free(cfg->field);          \
        cfg->field = NULL;                           \
    } while (0);
    LEUKO_RULES_LIST
#undef X

    /* Free AllCops lists */
    if (cfg->all_include)
    {
        for (size_t i = 0; i < cfg->all_include_count; i++)
        {
            free(cfg->all_include[i]);
        }
        free(cfg->all_include);
        cfg->all_include = NULL;
        cfg->all_include_count = 0;
    }
    if (cfg->all_exclude)
    {
        for (size_t i = 0; i < cfg->all_exclude_count; i++)
        {
            free(cfg->all_exclude[i]);
        }
        free(cfg->all_exclude);
        cfg->all_exclude = NULL;
        cfg->all_exclude_count = 0;
    }
}
