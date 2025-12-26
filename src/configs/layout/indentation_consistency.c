#include <string.h>
#include <stdlib.h>

#include "configs/common/rule_config.h"
#include "configs/layout/indentation_consistency.h"
#include "common/rule_registry.h"
#include "sources/node.h"

/**
 * @brief Initialize the layout_indentation_consistency rule configuration.
 * @return Pointer to the initialized leuko_rule_config_t structure
 */
leuko_rule_config_t *layout_indentation_consistency_initialize(void)
{
    /* Specific configuration */
    layout_indentation_consistency_config_t *specific_cfg = calloc(1, sizeof(*specific_cfg));
    if (!specific_cfg)
    {
        return NULL;
    }
    specific_cfg->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;

    /* Rule configuration */
    leuko_rule_config_t *cfg = leuko_rule_config_initialize();
    if (!cfg)
    {
        free(specific_cfg);
        return NULL;
    }
    cfg->specific_config = specific_cfg;
    cfg->specific_config_free = layout_indentation_consistency_config_free;
    return cfg;
}

/* New merged-node apply */
bool layout_indentation_consistency_apply_merged(leuko_rule_config_t *config, leuko_node_t *node, char **err)
{
    if (err)
        *err = NULL;
    if (!config || !config->specific_config || !node)
        return false;
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)config->specific_config;
    leuko_node_t *v = leuko_node_get_mapping_child(node, CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE);
    const char *val = v && LEUKO_NODE_IS_SCALAR(v->type) ? v->scalar : NULL;
    if (!val)
        return true; /* nothing to override */
    if (strcmp(val, CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS) == 0)
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS;
    else
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
    return true;
}

/**
 * @brief Free the memory allocated for a layout_indentation_consistency_config_t structure.
 * @param config Pointer to the layout_indentation_consistency_config_t structure to free
 */
void layout_indentation_consistency_config_free(void *config)
{
    if (!config)
    {
        return;
    }
    free(config);
}

/**
 * @brief Configuration operations for Layout/IndentationConsistency rule.
 */
struct leuko_rule_config_handlers_s layout_indentation_consistency_config_ops = {
    .initialize = layout_indentation_consistency_initialize,
    .apply = layout_indentation_consistency_apply_merged,
};
