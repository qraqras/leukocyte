#include "configs/common/rule_config.h"

/**
 * @brief Initialize a leuko_rule_config_t structure with default values.
 * @return Pointer to the initialized leuko_rule_config_t structure
 */
leuko_rule_config_t *leuko_rule_config_initialize(void)
{
    leuko_rule_config_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
    {
        return NULL;
    }
    cfg->enabled = true;
    cfg->severity = LEUKO_SEVERITY_CONVENTION;
    cfg->include = NULL;
    cfg->include_count = 0;
    cfg->exclude = NULL;
    cfg->exclude_count = 0;
    cfg->specific_config = NULL;
    cfg->specific_config_free = NULL;
    return cfg;
}
