#ifndef LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H
#define LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H

#include "configs/common/rule_config.h"

#define CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE "EnforcedStyle"
#define CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL "normal"
#define CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS "indented_internal_methods"

extern struct leuko_rule_config_handlers_s layout_indentation_consistency_config_ops;

/// @brief Indentation consistency enforced styles.
/// @note default: LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL
typedef enum
{
    LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL,
    LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS,
} indentation_consistency_enforced_style_t;

/// @brief Specific configuration for Layout/IndentationConsistency rule.
typedef struct layout_indentation_consistency_config_s
{
    indentation_consistency_enforced_style_t enforced_style;
} layout_indentation_consistency_config_t;

leuko_config_rule_view_t *layout_indentation_consistency_initialize(void);
/* New API: merged-node apply */
bool layout_indentation_consistency_apply_merged(leuko_config_rule_view_t *config, leuko_node_t *node, char **err);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H */
