/* Shared literals for Layout/IndentationConsistency */
#ifndef RULES_LAYOUT_INDENTATION_CONSISTENCY_H
#define RULES_LAYOUT_INDENTATION_CONSISTENCY_H
#include "configs/config.h"
#include <yaml.h>
#include "prism.h"

#define RULE_NAME_LAYOUT_INDENTATION_CONSISTENCY "Layout/IndentationConsistency"

#endif // RULES_LAYOUT_INDENTATION_CONSISTENCY_H

/* Optional config hooks for this rule */
rule_config_t *layout_indentation_consistency_create_default(void);
bool layout_indentation_consistency_apply_yaml(rule_config_t *config, const yaml_event_t *event, pm_parser_t *parser);
