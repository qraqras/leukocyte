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

leuko_rule_config_t *layout_indentation_consistency_initialize(void);
void layout_indentation_consistency_config_free(void *config);
/* Legacy: multi-doc apply */
bool layout_indentation_consistency_apply(leuko_rule_config_t *config, yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, char **err);
/* New API: merged-node apply */
bool layout_indentation_consistency_apply_merged(leuko_rule_config_t *config, leuko_yaml_node_t *merged, const char *full_name, const char *category_name, const char *rule_name, char **err);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H */
