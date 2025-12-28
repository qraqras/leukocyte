#include <string.h>
#include <stdlib.h>

#include "configs/common/rule_config.h"
#include "configs/common/category_config.h"
#include "configs/layout/indentation_consistency.h"
#include "common/rule_registry.h"
#include "sources/node.h"

/**
 * @brief Initialize the layout_indentation_consistency rule configuration.
 * @return Pointer to the initialized leuko_rule_config_t structure
 */
void *layout_indentation_consistency_initialize(void)
{
    leuko_config_rule_view_indentation_consistency_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    /* Initialize base defaults */
    cfg->base.enabled = true;
    cfg->base.severity = LEUKO_SEVERITY_CONVENTION;

    cfg->specific.enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
    return (void *)cfg;
}

void layout_indentation_consistency_reset(void *view)
{
    if (!view)
        return;
    free(view);
}

/* New merged-node apply */
bool layout_indentation_consistency_apply_merged(void *view, leuko_node_t *node, char **err)
{
    if (err)
        *err = NULL;
    if (!view || !node)
        return false;
    layout_indentation_consistency_config_t *sc = &((leuko_config_rule_view_indentation_consistency_t *)view)->specific;
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

/* No separate free required for typed embedded specifics. */

/**
 * @brief Configuration operations for Layout/IndentationConsistency rule.
 */
struct leuko_rule_config_handlers_s layout_indentation_consistency_config_ops = {
    .initialize = layout_indentation_consistency_initialize,
    .apply = layout_indentation_consistency_apply_merged,
    .reset = layout_indentation_consistency_reset,
};
