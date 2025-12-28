#ifndef LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_WIDTH_H
#define LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_WIDTH_H

#include "configs/common/rule_config.h"

#define CONFIG_KEY_OF_LAYOUT_INDENTATION_WIDTH "Width"

extern struct leuko_rule_config_handlers_s layout_indentation_width_config_ops;

/// @brief Specific configuration for Layout/IndentationWidth rule.
typedef struct layout_indentation_width_config_s
{
    int32_t width; /* default: 2 */
} layout_indentation_width_config_t;

void *layout_indentation_width_initialize(void);
bool layout_indentation_width_apply_merged(void *config, leuko_node_t *node, char **err);
void layout_indentation_width_reset(void *view);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_WIDTH_H */
