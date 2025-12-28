#include <stdlib.h>
#include <string.h>

#include "configs/common/category_config.h"
#include "configs/layout/indentation_style.h"
#include "sources/node.h"

void *layout_indentation_style_initialize(void)
{
    leuko_config_rule_view_indentation_style_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    /* Initialize base defaults */
    cfg->base.enabled = true;
    cfg->base.severity = LEUKO_SEVERITY_CONVENTION;

    cfg->specific.style = LAYOUT_INDENTATION_STYLE_SPACES;
    return (void *)cfg;
}

void layout_indentation_style_reset(void *view)
{
    if (!view)
        return;
    free(view);
}

bool layout_indentation_style_apply_merged(void *view, leuko_node_t *node, char **err)
{
    if (err)
        *err = NULL;
    if (!view || !node)
        return false;

    layout_indentation_style_config_t *sc = &((leuko_config_rule_view_indentation_style_t *)view)->specific;
    leuko_node_t *val_node = leuko_node_get_mapping_child(node, CONFIG_KEY_OF_LAYOUT_INDENTATION_STYLE);
    if (!val_node || !LEUKO_NODE_IS_SCALAR(val_node->type))
        return true; /* nothing to override */

    if (strcmp(val_node->scalar, CONFIG_VALUE_OF_LAYOUT_INDENTATION_STYLE_TABS) == 0)
    {
        sc->style = LAYOUT_INDENTATION_STYLE_TABS;
    }
    else if (strcmp(val_node->scalar, CONFIG_VALUE_OF_LAYOUT_INDENTATION_STYLE_SPACES) == 0)
    {
        sc->style = LAYOUT_INDENTATION_STYLE_SPACES;
    }
    else
    {
        if (err)
            *err = strdup("invalid IndentationStyle value");
        return false;
    }
    return true;
}

/* No separate free required for typed embedded specifics. */

struct leuko_rule_config_handlers_s layout_indentation_style_config_ops = {
    .initialize = layout_indentation_style_initialize,
    .apply = layout_indentation_style_apply_merged,
    .reset = layout_indentation_style_reset,
};
