#include <stdlib.h>
#include <string.h>

#include "configs/common/config.h"
#include "common/registry/registry.h"

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

    /* Initialize general and category pointers */
    cfg->general = NULL;
    cfg->categories = NULL;
    cfg->categories_count = 0;

    /* Backwards-compatible general arrays (kept in sync by materialize step) */
    cfg->general_include = NULL;
    cfg->general_include_count = 0;
    cfg->general_exclude = NULL;
    cfg->general_exclude_count = 0;

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

leuko_general_config_t *leuko_config_get_general_config(leuko_config_t *cfg)
{
    if (!cfg)
        return NULL;
    if (!cfg->general)
    {
        cfg->general = leuko_general_config_initialize();
    }
    return cfg->general;
}

leuko_category_config_t *leuko_config_get_category_config(leuko_config_t *cfg, const char *name)
{
    if (!cfg || !name)
        return NULL;
    for (size_t i = 0; i < cfg->categories_count; i++)
    {
        if (strcmp(cfg->categories[i]->name, name) == 0)
            return cfg->categories[i];
    }
    return NULL;
}

leuko_category_config_t *leuko_config_add_category(leuko_config_t *cfg, const char *name)
{
    if (!cfg || !name)
        return NULL;
    leuko_category_config_t *existing = leuko_config_get_category_config(cfg, name);
    if (existing)
        return existing;
    leuko_category_config_t *cc = leuko_category_config_initialize(name);
    if (!cc)
        return NULL;

    leuko_category_config_t **tmp = realloc(cfg->categories, sizeof(*tmp) * (cfg->categories_count + 1));
    if (!tmp)
    {
        leuko_category_config_free(cc);
        return NULL;
    }
    cfg->categories = tmp;
    cfg->categories[cfg->categories_count] = cc;
    cfg->categories_count++;
    return cc;
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

    /* Free general lists */
    if (cfg->general_include)
    {
        for (size_t i = 0; i < cfg->general_include_count; i++)
        {
            free(cfg->general_include[i]);
        }
        free(cfg->general_include);
        cfg->general_include = NULL;
        cfg->general_include_count = 0;
    }
    if (cfg->general_exclude)
    {
        for (size_t i = 0; i < cfg->general_exclude_count; i++)
        {
            free(cfg->general_exclude[i]);
        }
        free(cfg->general_exclude);
        cfg->general_exclude = NULL;
        cfg->general_exclude_count = 0;
    }

    /* Free any allocated category configs */    if (cfg->categories)
    {
        for (size_t i = 0; i < cfg->categories_count; i++)
        {
            leuko_category_config_free(cfg->categories[i]);
        }
        free(cfg->categories);
        cfg->categories = NULL;
        cfg->categories_count = 0;
    }

    /* Free general config if present */
    if (cfg->general)
    {
        leuko_general_config_free(cfg->general);
        cfg->general = NULL;
    }
}
