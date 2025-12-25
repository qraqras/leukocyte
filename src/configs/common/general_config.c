#include <stdlib.h>
#include <string.h>
#include "configs/common/general_config.h"

leuko_general_config_t *leuko_general_config_initialize(void)
{
    leuko_general_config_t *cfg = calloc(1, sizeof(*cfg));
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

void leuko_general_config_free(leuko_general_config_t *cfg)
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
