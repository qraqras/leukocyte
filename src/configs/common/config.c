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

    /* Initialize AllCops and category pointers */
    cfg->all_cops = NULL;
    cfg->categories = NULL;
    cfg->categories_count = 0;

    /* Backwards-compatible AllCops arrays (kept in sync by materialize step) */
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

static void leuko_all_cops_config_free(leuko_all_cops_config_t *acfg)
{
    if (!acfg)
        return;
    if (acfg->include)
    {
        for (size_t i = 0; i < acfg->include_count; i++)
            free(acfg->include[i]);
        free(acfg->include);
    }
    if (acfg->exclude)
    {
        for (size_t i = 0; i < acfg->exclude_count; i++)
            free(acfg->exclude[i]);
        free(acfg->exclude);
    }
    if (acfg->inherit_from)
        free(acfg->inherit_from);
    if (acfg->inherit_mode)
        free(acfg->inherit_mode);
    free(acfg);
}

static void leuko_category_config_free(leuko_category_config_t *ccfg)
{
    if (!ccfg)
        return;
    if (ccfg->name)
        free(ccfg->name);
    if (ccfg->include)
    {
        for (size_t i = 0; i < ccfg->include_count; i++)
            free(ccfg->include[i]);
        free(ccfg->include);
    }
    if (ccfg->exclude)
    {
        for (size_t i = 0; i < ccfg->exclude_count; i++)
            free(ccfg->exclude[i]);
        free(ccfg->exclude);
    }
    if (ccfg->inherit_mode)
        free(ccfg->inherit_mode);
    free(ccfg);
}

leuko_all_cops_config_t *leuko_config_all_cops(leuko_config_t *cfg)
{
    if (!cfg)
        return NULL;
    if (!cfg->all_cops)
    {
        cfg->all_cops = calloc(1, sizeof(*cfg->all_cops));
    }
    return cfg->all_cops;
}

leuko_category_config_t *leuko_config_get_category(leuko_config_t *cfg, const char *name)
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
    leuko_category_config_t *existing = leuko_config_get_category(cfg, name);
    if (existing)
        return existing;
    leuko_category_config_t *cc = calloc(1, sizeof(*cc));
    cc->name = strdup(name);
    cc->include = NULL;
    cc->include_count = 0;
    cc->exclude = NULL;
    cc->exclude_count = 0;
    cc->inherit_mode = NULL;

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

    /* Free any allocated category configs */
    if (cfg->categories)
    {
        for (size_t i = 0; i < cfg->categories_count; i++)
        {
            leuko_category_config_free(cfg->categories[i]);
        }
        free(cfg->categories);
        cfg->categories = NULL;
        cfg->categories_count = 0;
    }

    /* Free AllCops config if present */
    if (cfg->all_cops)
    {
        leuko_all_cops_config_free(cfg->all_cops);
        cfg->all_cops = NULL;
    }
}
