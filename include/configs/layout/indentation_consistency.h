#ifndef LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H
#define LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H

#include "configs/config.h"
#include "configs/config_ops.h"

#define CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE "EnforcedStyle"
#define CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL "normal"
#define CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS "indented_internal_methods"

extern const struct config_ops layout_indentation_consistency_config_ops;

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

rule_config_t *layout_indentation_consistency_initialize(void);
bool layout_indentation_consistency_apply(rule_config_t *config, const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, pm_list_t *diagnostics);
void layout_indentation_consistency_config_free(void *config);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H */
