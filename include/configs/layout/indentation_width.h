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

leuko_rule_config_t *layout_indentation_width_initialize(void);
void layout_indentation_width_config_free(void *config);
/* New API: merged-node apply */
bool layout_indentation_width_apply_merged(leuko_rule_config_t *config, leuko_node_t *merged, const char *full_name, const char *category_name, const char *rule_name, char **err);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_WIDTH_H */
