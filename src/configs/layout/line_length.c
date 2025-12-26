#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/layout/line_length.h"
#include "sources/node.h"

leuko_rule_config_t *layout_line_length_initialize(void)
{
    layout_line_length_config_t *specific = calloc(1, sizeof(*specific));
    if (!specific)
        return NULL;
    specific->max = 80; /* default */

    leuko_rule_config_t *cfg = leuko_rule_config_initialize();
    if (!cfg)
    {
        free(specific);
        return NULL;
    }
    cfg->specific_config = specific;
    cfg->specific_config_free = layout_line_length_config_free;
    return cfg;
}

bool layout_line_length_apply_merged(leuko_rule_config_t *config, leuko_node_t *merged, const char *full_name, const char *category_name, const char *rule_name, char **err)
{
    (void)category_name;
    (void)rule_name;
    if (err)
        *err = NULL;
    if (!config || !config->specific_config || !merged)
        return false;

    layout_line_length_config_t *sc = (layout_line_length_config_t *)config->specific_config;
    leuko_node_t *node = (leuko_node_t *)merged;
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

void layout_line_length_config_free(void *config)
{
    if (!config)
        return;
    free(config);
}

struct leuko_rule_config_handlers_s layout_line_length_config_ops = {
    .initialize = layout_line_length_initialize,
    .apply_merged = layout_line_length_apply_merged,
};