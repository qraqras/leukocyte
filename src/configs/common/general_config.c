#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "configs/common/general_config.h"
#include "configs/common/config.h"
#include "sources/node.h"
#include "utils/glob_to_regex.h"

leuko_config_general_t *leuko_general_config_initialize(void)
{
    leuko_config_general_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
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

void leuko_general_config_free(leuko_config_general_t *cfg)
{
    if (!cfg)
        return;
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

bool leuko_general_config_apply(leuko_config_t *cfg, leuko_node_t *general)
{
    if (!cfg || !general)
        return false;

    leuko_node_t *inc = leuko_node_get_mapping_child(general, LEUKO_CONFIG_KEY_INCLUDE);
    size_t n = leuko_node_array_count(inc);
    if (n > 0)
    {
        leuko_config_general_t *ac = leuko_config_get_general_config(cfg);
        ac->include = calloc(n, sizeof(char *));
        ac->include_count = n;
        for (size_t i = 0; i < n; i++)
            ac->include[i] = strdup(leuko_node_array_scalar_at(inc, i) ?: "");

        /* compile regexes */
        ac->include_re = calloc(n, sizeof(regex_t));
        size_t compiled = 0;
        for (size_t i = 0; i < n; i++)
        {
            char *re_s = leuko_glob_to_regex(ac->include[i]);
            if (!re_s)
                continue;
            if (regcomp(&ac->include_re[compiled], re_s, REG_EXTENDED | REG_NOSUB) == 0)
                compiled++;
            free(re_s);
        }
        ac->include_re_count = compiled;
    }

    leuko_node_t *exc = leuko_node_get_mapping_child(general, LEUKO_CONFIG_KEY_EXCLUDE);
    n = leuko_node_array_count(exc);
    if (n > 0)
    {
        leuko_config_general_t *ac = leuko_config_get_general_config(cfg);
        ac->exclude = calloc(n, sizeof(char *));
        ac->exclude_count = n;
        for (size_t i = 0; i < n; i++)
            ac->exclude[i] = strdup(leuko_node_array_scalar_at(exc, i) ?: "");

        ac->exclude_re = calloc(n, sizeof(regex_t));
        size_t compiled = 0;
        for (size_t i = 0; i < n; i++)
        {
            char *re_s = leuko_glob_to_regex(ac->exclude[i]);
            if (!re_s)
                continue;
            if (regcomp(&ac->exclude_re[compiled], re_s, REG_EXTENDED | REG_NOSUB) == 0)
                compiled++;
            free(re_s);
        }
        ac->exclude_re_count = compiled;
    }
    return true;
}
