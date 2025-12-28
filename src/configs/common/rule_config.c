#include <stdlib.h>
#include <string.h>
#include "configs/common/rule_config.h"
#include "configs/common/config.h"

/**
 * @brief Initialize a leuko_rule_config_t structure with default values.
 * @return Pointer to the initialized leuko_rule_config_t structure
 */
void *leuko_rule_config_initialize(void)
{
    leuko_config_rule_view_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
    {
        return NULL;
    }
    cfg->base.enabled = true;
    cfg->base.severity = LEUKO_SEVERITY_CONVENTION;
    cfg->base.include = NULL;
    cfg->base.include_count = 0;
    cfg->base.exclude = NULL;
    cfg->base.exclude_count = 0;

    cfg->base.include_re = NULL;
    cfg->base.include_re_count = 0;
    cfg->base.exclude_re = NULL;
    cfg->base.exclude_re_count = 0;

    /* typed-specifics are embedded per-rule; base view no longer holds a generic pointer */
    return (void *)cfg;
}

/**
 * @brief Free a leuko_rule_config_t structure.
 * @param cfg Pointer to the leuko_rule_config_t structure to free
 */
void leuko_rule_config_free(void *cfg)
{
    if (!cfg)
        return;
    leuko_rule_config_reset(cfg);
    free(cfg);
}

/* Reset an embedded rule struct without freeing the struct itself. */
void leuko_rule_config_reset(void *cfg)
{
    if (!cfg)
        return;
    /* Reset base fields */
    leuko_rule_config_base_reset(&((leuko_config_rule_view_t *)cfg)->base);
    /* Reset specific: typed specifics are embedded; call leuko_rule_config_view_reset for typed resets if needed */
    /* No generic pointer to free here */
}

/* Reset an embedded base struct */
void leuko_rule_config_base_reset(leuko_config_rule_base_t *base)
{
    if (!base)
        return;
    if (base->include_re)
    {
        for (size_t i = 0; i < base->include_re_count; i++)
            regfree(&base->include_re[i]);
        free(base->include_re);
        base->include_re = NULL;
        base->include_re_count = 0;
    }
    if (base->exclude_re)
    {
        for (size_t i = 0; i < base->exclude_re_count; i++)
            regfree(&base->exclude_re[i]);
        free(base->exclude_re);
        base->exclude_re = NULL;
        base->exclude_re_count = 0;
    }

    if (base->include)
    {
        for (size_t i = 0; i < base->include_count; i++)
            free(base->include[i]);
        free(base->include);
        base->include = NULL;
        base->include_count = 0;
    }
    if (base->exclude)
    {
        for (size_t i = 0; i < base->exclude_count; i++)
            free(base->exclude[i]);
        free(base->exclude);
        base->exclude = NULL;
        base->exclude_count = 0;
    }
}

/* Reset a full view (base + specific) */
void leuko_rule_config_view_reset(void *view)
{
    if (!view)
        return;
    leuko_rule_config_base_reset(&((leuko_config_rule_view_t *)view)->base);
    /* No generic specific pointer to free; typed specifics remain embedded and should be reset by rule-specific logic if needed. */
}

/* Move heap-allocated rule config into embedded view and free the source */
void leuko_rule_config_move_to_view(void *src, void *dst)
{
    if (!src || !dst)
        return;
    /* Reset existing dst */
    leuko_rule_config_view_reset(dst);

    /* Move scalar fields into dst->base */
    ((leuko_config_rule_view_t *)dst)->base.enabled = ((leuko_config_rule_view_t *)src)->base.enabled;
    ((leuko_config_rule_view_t *)dst)->base.severity = ((leuko_config_rule_view_t *)src)->base.severity;

    ((leuko_config_rule_view_t *)dst)->base.include = ((leuko_config_rule_view_t *)src)->base.include;
    ((leuko_config_rule_view_t *)dst)->base.include_count = ((leuko_config_rule_view_t *)src)->base.include_count;
    ((leuko_config_rule_view_t *)dst)->base.include_re = ((leuko_config_rule_view_t *)src)->base.include_re;
    ((leuko_config_rule_view_t *)dst)->base.include_re_count = ((leuko_config_rule_view_t *)src)->base.include_re_count;

    ((leuko_config_rule_view_t *)dst)->base.exclude = ((leuko_config_rule_view_t *)src)->base.exclude;
    ((leuko_config_rule_view_t *)dst)->base.exclude_count = ((leuko_config_rule_view_t *)src)->base.exclude_count;
    ((leuko_config_rule_view_t *)dst)->base.exclude_re = ((leuko_config_rule_view_t *)src)->base.exclude_re;
    ((leuko_config_rule_view_t *)dst)->base.exclude_re_count = ((leuko_config_rule_view_t *)src)->base.exclude_re_count;

    /* Note: typed-specifics are handled by typed move logic (e.g., leuko_config_set_view_rule). */

    /* Null out source base pointers so freeing it won't free moved memory */
    ((leuko_config_rule_view_t *)src)->base.include = NULL;
    ((leuko_config_rule_view_t *)src)->base.include_count = 0;
    ((leuko_config_rule_view_t *)src)->base.include_re = NULL;
    ((leuko_config_rule_view_t *)src)->base.include_re_count = 0;
    ((leuko_config_rule_view_t *)src)->base.exclude = NULL;
    ((leuko_config_rule_view_t *)src)->base.exclude_count = 0;
    ((leuko_config_rule_view_t *)src)->base.exclude_re = NULL;
    ((leuko_config_rule_view_t *)src)->base.exclude_re_count = 0;

    /* Free source object (typed heap view) */
    leuko_rule_config_free(src);
}

#include <regex.h>
#include "sources/node.h"
#include "common/rule_registry.h"
#include "utils/glob_to_regex.h"

bool leuko_rule_config_apply(void *rconf, const struct leuko_registry_rule_entry_s *ent, leuko_node_t *rule_node, char **err)
{
    if (!rconf || !ent)
    {
        if (err)
            *err = strdup("Invalid arguments");
        return false;
    }

    if (rule_node)
    {
        leuko_node_t *inc = leuko_node_get_mapping_child(rule_node, LEUKO_CONFIG_KEY_INCLUDE);
        size_t n = leuko_node_array_count(inc);
        if (n > 0)
        {
            ((leuko_config_rule_view_t *)rconf)->base.include = calloc(n, sizeof(char *));
            ((leuko_config_rule_view_t *)rconf)->base.include_count = n;
            for (size_t i = 0; i < n; i++)
                ((leuko_config_rule_view_t *)rconf)->base.include[i] = strdup(leuko_node_array_scalar_at(inc, i) ?: "");

            ((leuko_config_rule_view_t *)rconf)->base.include_re = calloc(n, sizeof(regex_t));
            size_t compiled = 0;
            for (size_t i = 0; i < n; i++)
            {
                char *re_s = leuko_glob_to_regex(((leuko_config_rule_view_t *)rconf)->base.include[i]);
                if (!re_s)
                    continue;
                if (regcomp(&((leuko_config_rule_view_t *)rconf)->base.include_re[compiled], re_s, REG_EXTENDED | REG_NOSUB) == 0)
                    compiled++;
                free(re_s);
            }
            ((leuko_config_rule_view_t *)rconf)->base.include_re_count = compiled;
        }
        leuko_node_t *exc = leuko_node_get_mapping_child(rule_node, LEUKO_CONFIG_KEY_EXCLUDE);
        n = leuko_node_array_count(exc);
        if (n > 0)
        {
            ((leuko_config_rule_view_t *)rconf)->base.exclude = calloc(n, sizeof(char *));
            ((leuko_config_rule_view_t *)rconf)->base.exclude_count = n;
            for (size_t i = 0; i < n; i++)
                ((leuko_config_rule_view_t *)rconf)->base.exclude[i] = strdup(leuko_node_array_scalar_at(exc, i) ?: "");

            ((leuko_config_rule_view_t *)rconf)->base.exclude_re = calloc(n, sizeof(regex_t));
            size_t compiled = 0;
            for (size_t i = 0; i < n; i++)
            {
                char *re_s = leuko_glob_to_regex(((leuko_config_rule_view_t *)rconf)->base.exclude[i]);
                if (!re_s)
                    continue;
                if (regcomp(&((leuko_config_rule_view_t *)rconf)->base.exclude_re[compiled], re_s, REG_EXTENDED | REG_NOSUB) == 0)
                    compiled++;
                free(re_s);
            }
            ((leuko_config_rule_view_t *)rconf)->base.exclude_re_count = compiled;
        }
    }

    const leuko_rule_config_handlers_t *ops = ent->handlers;
    if (ops && ops->apply)
    {
        leuko_node_t *arg_node = rule_node;
        bool ok = ops->apply(rconf, arg_node, err);
        return ok;
    }
    return true;
}
