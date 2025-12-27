#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "configs/common/category_config.h"
#include "configs/common/config.h"

leuko_config_category_t *leuko_category_config_initialize(const char *name)
{
    leuko_config_category_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    if (name)
        cfg->name = strdup(name);
    else
        cfg->name = NULL;
    cfg->include = NULL;
    cfg->include_count = 0;
    cfg->exclude = NULL;
    cfg->exclude_count = 0;

    cfg->include_re = NULL;
    cfg->include_re_count = 0;
    cfg->exclude_re = NULL;
    cfg->exclude_re_count = 0;

    return cfg;
}

void leuko_category_config_free(leuko_config_category_t *cfg)
{
    if (!cfg)
        return;
    if (cfg->name)
        free(cfg->name);

    if (cfg->include_re)
    {
        for (size_t i = 0; i < cfg->include_re_count; i++)
            regfree(&cfg->include_re[i]);
        free(cfg->include_re);
        cfg->include_re = NULL;
        cfg->include_re_count = 0;
    }
    if (cfg->exclude_re)
    {
        for (size_t i = 0; i < cfg->exclude_re_count; i++)
            regfree(&cfg->exclude_re[i]);
        free(cfg->exclude_re);
        cfg->exclude_re = NULL;
        cfg->exclude_re_count = 0;
    }

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

    free(cfg);
}

/* Reset an embedded category struct without freeing the struct itself. */
void leuko_category_config_reset(leuko_config_category_t *cfg)
{
    if (!cfg)
        return;
    if (cfg->name)
    {
        free(cfg->name);
        cfg->name = NULL;
    }

    if (cfg->include_re)
    {
        for (size_t i = 0; i < cfg->include_re_count; i++)
            regfree(&cfg->include_re[i]);
        free(cfg->include_re);
        cfg->include_re = NULL;
        cfg->include_re_count = 0;
    }
    if (cfg->exclude_re)
    {
        for (size_t i = 0; i < cfg->exclude_re_count; i++)
            regfree(&cfg->exclude_re[i]);
        free(cfg->exclude_re);
        cfg->exclude_re = NULL;
        cfg->exclude_re_count = 0;
    }

    if (cfg->include)
    {
        for (size_t i = 0; i < cfg->include_count; i++)
            free(cfg->include[i]);
        free(cfg->include);
        cfg->include = NULL;
        cfg->include_count = 0;
    }
    if (cfg->exclude)
    {
        for (size_t i = 0; i < cfg->exclude_count; i++)
            free(cfg->exclude[i]);
        free(cfg->exclude);
        cfg->exclude = NULL;
        cfg->exclude_count = 0;
    }
}

#include <regex.h>
#include "sources/node.h"
#include "utils/glob_to_regex.h"

#include "common/rule_registry.h"
#include "configs/common/rule_config.h"

bool leuko_category_config_apply(leuko_config_t *cfg, const char *name, leuko_node_t *cnode)
{
    if (!cfg || !name || !cnode)
        return false;
    /* Only apply categories that exist in the registry (Leuko-supported). */
    const leuko_registry_category_t *cat = leuko_rule_find_category(name, NULL);
    if (!cat)
        return true; /* ignore unknown categories */

    /* Only operate on categories that already exist in the runtime config.
     * If the category hasn't been added previously, it is unknown to Leuko and
     * we must not create it here. */
    leuko_config_category_t *cc = leuko_config_get_category_config(cfg, name);
    if (!cc)
        return true;

    leuko_node_t *cinc = leuko_node_get_mapping_child(cnode, LEUKO_CONFIG_KEY_INCLUDE);
    size_t n = leuko_node_array_count(cinc);
    if (n > 0)
    {
        cc->include = calloc(n, sizeof(char *));
        if (!cc->include)
            return false;
        cc->include_count = n;
        for (size_t j = 0; j < n; j++)
            cc->include[j] = strdup(leuko_node_array_scalar_at(cinc, j) ?: "");

        cc->include_re = calloc(n, sizeof(regex_t));
        size_t compiled = 0;
        for (size_t i = 0; i < n; i++)
        {
            char *re_s = leuko_glob_to_regex(cc->include[i]);
            if (!re_s)
                continue;
            if (regcomp(&cc->include_re[compiled], re_s, REG_EXTENDED | REG_NOSUB) == 0)
                compiled++;
            free(re_s);
        }
        cc->include_re_count = compiled;
    }

    leuko_node_t *cexc = leuko_node_get_mapping_child(cnode, LEUKO_CONFIG_KEY_EXCLUDE);
    n = leuko_node_array_count(cexc);
    if (n > 0)
    {
        cc->exclude = calloc(n, sizeof(char *));
        if (!cc->exclude)
            return false;
        cc->exclude_count = n;
        for (size_t j = 0; j < n; j++)
            cc->exclude[j] = strdup(leuko_node_array_scalar_at(cexc, j) ?: "");

        cc->exclude_re = calloc(n, sizeof(regex_t));
        size_t compiled = 0;
        for (size_t i = 0; i < n; i++)
        {
            char *re_s = leuko_glob_to_regex(cc->exclude[i]);
            if (!re_s)
                continue;
            if (regcomp(&cc->exclude_re[compiled], re_s, REG_EXTENDED | REG_NOSUB) == 0)
                compiled++;
            free(re_s);
        }
        cc->exclude_re_count = compiled;
    }

    /* Apply rules mapping under this category node (if present). If 'cat' is NULL,
     * only include/exclude are applied and rules are skipped.
     */
    leuko_node_t *rules = leuko_node_get_mapping_child(cnode, "rules");
    if (rules && rules->type == LEUKO_NODE_OBJECT)
    {
        for (size_t ri = 0; ri < rules->map_len; ri++)
        {
            const char *rule_key = rules->map_keys[ri];
            if (!rule_key)
                continue;
            leuko_node_t *rule_node = rules->map_vals[ri];
            if (!rule_node || rule_node->type != LEUKO_NODE_OBJECT)
                continue;

            const leuko_registry_rule_entry_t *ent = NULL;
            for (size_t k = 0; k < cat->count; k++)
            {
                if (cat->entries[k].name && strcmp(cat->entries[k].name, rule_key) == 0)
                {
                    ent = &cat->entries[k];
                    break;
                }
            }
            if (!ent)
                continue;

            leuko_config_rule_view_t *rconf = NULL;
            if (ent->handlers && ent->handlers->initialize)
            {
                rconf = ent->handlers->initialize();
            }
            else
            {
                /* Do not create default rule configs; only rules with a specific initialize
                 * handler are considered valid for materialization. */
                continue;
            }
            if (!rconf)
                continue;

            char *err = NULL;
            if (!leuko_rule_config_apply(rconf, ent, rule_node, &err))
            {
                if (err)
                    free(err);
                leuko_rule_config_free(rconf);
                continue;
            }

            if (!cc)
            {
                /* If the category config isn't present, we must not create it here. */
                leuko_rule_config_free(rconf);
                continue;
            }

            /* Update static view convenience pointers. The rule is not stored in
             * per-category arrays (we centralize rule storage in the generated
             * `categories` view). */
            leuko_config_set_view_rule(cfg, name, rule_key, rconf);
        }
    }

    return true;
}
