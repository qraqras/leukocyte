#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/common/config.h"
#include "configs/common/rule_config.h"
#include "common/rule_registry.h"

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

    /* Initialize general pointer */
    cfg->general = NULL;

    /* Initialize general config now (must be present) */
    cfg->general = leuko_general_config_initialize();
    if (!cfg->general)
        return;

    /* Categories are embedded in `categories` (no dynamic array). */

/* Validate registry entries: ensure each rule provides initialize() and apply(). */
    {
        const leuko_registry_category_t *cats = leuko_get_rule_categories();
        size_t ncat = leuko_get_rule_category_count();
        for (size_t ci = 0; ci < ncat; ci++)
        {
            const leuko_registry_category_t *cat = &cats[ci];
            for (size_t ei = 0; ei < cat->count; ei++)
            {
                const leuko_registry_rule_entry_t *ent = &cat->entries[ei];
                if (!ent->handlers || !ent->handlers->initialize || !ent->handlers->apply)
                {
                    fprintf(stderr, "Registry entry for %s/%s missing initialize/apply\n", cat->name, ent->name);
                    abort();
                }
            }
        }
    }

    /* Initialize embedded category views' base structs for all known categories.
     * We map registry category names (e.g., "Layout") to the generated view
     * member and initialize the embedded `leuko_config_category_t`'s name.
     */
    {
        const leuko_registry_category_t *cats = leuko_get_rule_categories();
        size_t ncat = leuko_get_rule_category_count();
        for (size_t i = 0; i < ncat; i++)
        {
            leuko_config_category_view_t *v = leuko_config_get_view_category(cfg, cats[i].name);
            if (v)
            {
                /* ensure any previous data is clean */
                leuko_category_config_reset(&v->base);
                v->base.name = strdup(cats[i].name);
            }
        }
    }
}

leuko_config_general_t *leuko_config_get_general_config(leuko_config_t *cfg)
{
    if (!cfg)
        return NULL;
    if (!cfg->general)
    {
        cfg->general = leuko_general_config_initialize();
    }
    return cfg->general;
}

leuko_config_category_view_t *leuko_config_get_view_category(leuko_config_t *cfg, const char *name)
{
    if (!cfg || !name)
        return NULL;
    if (strcmp(name, "Layout") == 0)
        return &cfg->categories.layout;
    if (strcmp(name, "Lint") == 0)
        return &cfg->categories.lint;
    return NULL;
}

leuko_config_category_t *leuko_config_get_category_config(leuko_config_t *cfg, const char *name)
{
    leuko_config_category_view_t *v = leuko_config_get_view_category(cfg, name);
    if (!v)
        return NULL;
    return &v->base;
}


/* Helper: get a rule config by category and rule name. This returns the
 * pointer stored in the generated static `categories` view. */
leuko_config_rule_base_t *leuko_config_get_rule(leuko_config_t *cfg, const char *category, const char *rule_name)
{
    if (!cfg || !category || !rule_name)
        return NULL;

    /* Expand known categories and their rules; return pointer to embedded base. */
    if (strcmp(category, "Layout") == 0)
    {
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
        if (strcmp(rule_name, sname) == 0) \
            return &cfg->categories.layout.rules.field.base;
        LEUKO_RULES_LAYOUT
#undef X
    }

    return NULL;
}

/* Return a pointer to the view-type for a rule config (includes specific_config). */
leuko_config_rule_view_t *leuko_config_get_view_rule(leuko_config_t *cfg, const char *category, const char *rule_name)
{
    if (!cfg || !category || !rule_name)
        return NULL;

    if (strcmp(category, "Layout") == 0)
    {
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
        if (strcmp(rule_name, sname) == 0) \
            return &cfg->categories.layout.rules.field;
        LEUKO_RULES_LAYOUT
#undef X
    }
    return NULL;
}

/* Set the static view pointer for convenience access; generated names map to view fields. */
void leuko_config_set_view_rule(leuko_config_t *cfg, const char *category, const char *rule_name, leuko_config_rule_view_t *rconf)
{
    if (!cfg || !category || !rule_name || !rconf)
        return;
    /* Generated mapping from registry: move rconf's contents into embedded base field */
    if (strcmp(category, "Layout") == 0)
    {
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
        if (strcmp(rule_name, sname) == 0) \
        { \
            leuko_rule_config_move_to_view(rconf, &cfg->categories.layout.rules.field); \
            return; \
        }
        LEUKO_RULES_LAYOUT
#undef X
    }
}

/**
 * @brief Get a pointer to a leuko_config_rule_view_t field by its index.
 * @param cfg Pointer to the leuko_config_t structure
 * @param idx Index of the leuko_config_rule_view_t field
 * @return Pointer to the leuko_config_rule_view_t field, or NULL if not found
 */


/**
 * @brief Get the total number of leuko_config_rule_view_t fields in leuko_config_t.
 * @return Total number of leuko_config_rule_view_t fields
 */
size_t leuko_config_count(void)
{
    size_t total = 0;
    const leuko_registry_category_t *cats = leuko_get_rule_categories();
    size_t ncat = leuko_get_rule_category_count();
    for (size_t i = 0; i < ncat; i++)
        total += cats[i].count;
    return total;
}

/**
 * @brief Free a leuko_config_rule_view_t structure.
 * @param cfg Pointer to the leuko_config_rule_view_t structure to free
 */

/**
 * @brief Free a leuko_config_t structure.
 * @param cfg Pointer to the leuko_config_t structure to free
 */
leuko_config_t *leuko_config_new(void)
{
    leuko_config_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    leuko_config_initialize(cfg);
    return cfg;
}

void leuko_config_free(leuko_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }

/* flat per-rule fields removed — runtime per-category configs are freed below */


    /* Free embedded category configs (reset internal allocations). */
    {
        const leuko_registry_category_t *cats = leuko_get_rule_categories();
        size_t ncat = leuko_get_rule_category_count();
        for (size_t i = 0; i < ncat; i++)
        {
            leuko_config_category_view_t *v = leuko_config_get_view_category(cfg, cats[i].name);
            if (v)
                leuko_category_config_reset(&v->base);
        }
    }

    /* Free general config if present */
    if (cfg->general)
    {
        leuko_general_config_free(cfg->general);
        cfg->general = NULL;
    }
}
