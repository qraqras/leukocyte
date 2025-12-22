#ifndef LEUKOCYTE_RULE_H
#define LEUKOCYTE_RULE_H

#include <stdbool.h>
#include "prism.h"
#include "rule_registry.h"
// Forward-declare config type to avoid circular include
typedef struct leuko_config_s leuko_config_t;

// Define the number of node types (Prism doesn't define PM_NODE_TYPE_COUNT, so we use a large number)
#define PM_NODE_TYPE_COUNT 256

// Rule struct (tagged so it can be forward-declared from other headers)
#include "rules/rule_context.h"

typedef struct rule_s
{
    bool (*handlers[PM_NODE_TYPE_COUNT])(pm_node_t *node, const rule_context_t *ctx); // Handlers per node type
} rule_t;

// External rule declarations
extern rule_t layout_indentation_consistency_rule;

#endif /* LEUKOCYTE_RULE_H */
