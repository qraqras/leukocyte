#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/layout/indentation_width.h"
#include "sources/node.h"

leuko_rule_config_t *layout_indentation_width_initialize(void)
{
    layout_indentation_width_config_t *specific = calloc(1, sizeof(*specific));
    if (!specific)
        return NULL;
    specific->width = 2; /* default */

    leuko_rule_config_t *cfg = leuko_rule_config_initialize();
    if (!cfg)
    {
        free(specific);
        return NULL;
    }
    cfg->specific_config = specific;
    cfg->specific_config_free = layout_indentation_width_config_free;
    return cfg;
}

bool layout_indentation_width_apply_merged(leuko_rule_config_t *config, leuko_node_t *node, char **err)
{
    if (err)
        *err = NULL;
    if (!config || !config->specific_config || !node)
        return false;

    layout_indentation_width_config_t *sc = (layout_indentation_width_config_t *)config->specific_config;
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

void layout_indentation_width_config_free(void *config)
{
    if (!config)
        return;
    free(config);
}

struct leuko_rule_config_handlers_s layout_indentation_width_config_ops = {
    .initialize = layout_indentation_width_initialize,
    .apply = layout_indentation_width_apply_merged,
};
