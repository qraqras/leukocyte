#ifndef LEUKOCYTE_RULE_H
#define LEUKOCYTE_RULE_H

#include <stdbool.h>
#include "prism.h" // pm_node_type_t, pm_node_t, pm_parser_t, pm_diagnostic_list_t
// Forward-declare config type to avoid circular include
typedef struct config_s config_t;

// Define the number of node types (Prism doesn't define PM_NODE_TYPE_COUNT, so we use a large number)
#define PM_NODE_TYPE_COUNT 256

// Rule Category enum
typedef enum
{
    RULE_CATEGORY_LAYOUT,
    RULE_CATEGORY_LINT,
} rule_category_t;

// Rule struct
typedef struct
{
    const char *name;                                                                                                  // Rule name (e.g., "Layout/Indentation")
    rule_category_t category;                                                                                          // Category
    bool enabled;                                                                                                      // Enabled flag
    bool (*handlers[PM_NODE_TYPE_COUNT])(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg); // Handlers per node type
} rule_t;

bool rule_category_from_string(const char *str, rule_category_t *out);
bool rule_category_to_string(rule_category_t category, const char **out);

// External rule declarations
extern rule_t layout_indentation_consistency_rule;

#endif // LEUKOCYTE_RULE_H
