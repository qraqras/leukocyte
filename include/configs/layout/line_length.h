#ifndef LEUKOCYTE_CONFIGS_LAYOUT_LINE_LENGTH_H
#define LEUKOCYTE_CONFIGS_LAYOUT_LINE_LENGTH_H

#include "configs/common/rule_config.h"

#define CONFIG_KEY_OF_LAYOUT_LINE_LENGTH_MAX "Max"

extern struct leuko_rule_config_handlers_s layout_line_length_config_ops;

typedef struct layout_line_length_config_s
{
    int32_t max; /* default: 80 */
} layout_line_length_config_t;

leuko_config_rule_view_t *layout_line_length_initialize(void);
/* New API: merged-node apply */
bool layout_line_length_apply_merged(leuko_config_rule_view_t *config, leuko_node_t *node, char **err);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_LINE_LENGTH_H */
