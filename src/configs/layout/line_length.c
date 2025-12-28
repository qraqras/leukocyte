#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/common/category_config.h"
#include "configs/layout/line_length.h"
#include "sources/node.h"

void *layout_line_length_initialize(void)
{
    leuko_config_rule_view_line_length_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    /* base defaults */
    cfg->base.enabled = true;
    cfg->base.severity = LEUKO_SEVERITY_CONVENTION;

    cfg->specific.max = 80; /* default */
    return (void *)cfg;
}

void layout_line_length_reset(void *view)
{
    if (!view)
        return;
    free(view);
}

bool layout_line_length_apply_merged(void *view, leuko_node_t *node, char **err)
{
    if (err)
        *err = NULL;
    if (!view || !node)
        return false;

    layout_line_length_config_t *sc = &((leuko_config_rule_view_line_length_t *)view)->specific;
    leuko_node_t *val_node = leuko_node_get_mapping_child(node, CONFIG_KEY_OF_LAYOUT_LINE_LENGTH_MAX);
    if (!val_node || !LEUKO_NODE_IS_SCALAR(val_node->type))
        return true; /* nothing to override */

    char *endptr = NULL;
    long v = strtol(val_node->scalar, &endptr, 10);
    if (!val_node->scalar || endptr == val_node->scalar || *endptr != '\0' || v <= 0)
    {
        if (err)
            *err = strdup("invalid Max value");
        return false;
    }
    sc->max = (int32_t)v;
    return true;
}

/* No separate free required for typed embedded specifics. */

struct leuko_rule_config_handlers_s layout_line_length_config_ops = {
    .initialize = layout_line_length_initialize,
    .apply = layout_line_length_apply_merged,
    .reset = layout_line_length_reset,
};
