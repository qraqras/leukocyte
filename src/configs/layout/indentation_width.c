#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/common/category_config.h"
#include "configs/layout/indentation_width.h"
#include "sources/node.h"

void *layout_indentation_width_initialize(void)
{
    leuko_config_rule_view_indentation_width_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    /* Initialize base defaults (same as leuko_rule_config_initialize) */
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

    cfg->specific.width = 2; /* default */
    return (void *)cfg;
}

void layout_indentation_width_reset(void *view)
{
    if (!view)
        return;
    free(view);
}

bool layout_indentation_width_apply_merged(void *view, leuko_node_t *node, char **err)
{
    if (err)
        *err = NULL;
    if (!view || !node)
        return false;

    layout_indentation_width_config_t *sc = &((leuko_config_rule_view_indentation_width_t *)view)->specific;
    leuko_node_t *val_node = leuko_node_get_mapping_child(node, CONFIG_KEY_OF_LAYOUT_INDENTATION_WIDTH);
    if (!val_node || !LEUKO_NODE_IS_SCALAR(val_node->type))
        return true; /* nothing to override */

    char *endptr = NULL;
    long v = strtol(val_node->scalar, &endptr, 10);
    if (!val_node->scalar || endptr == val_node->scalar || *endptr != '\0' || v <= 0)
    {
        if (err)
            *err = strdup("invalid Width value");
        return false;
    }
    sc->width = (int32_t)v;
    return true;
}

/* No separate free required for typed embedded specifics. */

struct leuko_rule_config_handlers_s layout_indentation_width_config_ops = {
    .initialize = layout_indentation_width_initialize,
    .apply = layout_indentation_width_apply_merged,
    .reset = layout_indentation_width_reset,
};
