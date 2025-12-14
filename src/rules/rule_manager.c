#include "rules/rule.h"
#include <stdlib.h>
#include <string.h>

// Array of rules by node type
rule_t **rules_by_type[PM_NODE_TYPE_COUNT];
size_t rules_count_by_type[PM_NODE_TYPE_COUNT];

// Struct to pass data to visitor
typedef struct
{
    pm_parser_t *parser;
    pm_list_t *diagnostics;
    config_t *cfg;
} visit_data_t;

// Visitor function for each node
bool node_visitor(const pm_node_t *node, void *data)
{
    visit_data_t *visit_data = (visit_data_t *)data;
    pm_node_type_t type = node->type;

    // Call handlers for this node type
    if (rules_count_by_type[type] > 0)
    {
        for (size_t i = 0; i < rules_count_by_type[type]; i++)
        {
            rule_t *rule = rules_by_type[type][i];
                if (rule->enabled && rule->handlers[type])
                {
                    rule->handlers[type]((pm_node_t *)node, visit_data->parser, visit_data->diagnostics, visit_data->cfg);
                }
        }
    }

    return true; // Continue visiting
}

// Visitor function for AST traversal
bool visit_node(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg)
{
    visit_data_t data = {parser, diagnostics, cfg};
    pm_visit_node(node, node_visitor, &data);
    return true;
}

// Initialize rules (collect enabled rules by node type)
void init_rules()
{
    // Initialize empty
    memset(rules_by_type, 0, sizeof(rules_by_type));
    memset(rules_count_by_type, 0, sizeof(rules_count_by_type));

    // Add Layout/Indentation rule
    rules_by_type[PM_DEF_NODE] = malloc(sizeof(rule_t *));
    rules_by_type[PM_DEF_NODE][0] = &layout_indentation_consistency_rule;
    rules_count_by_type[PM_DEF_NODE] = 1;
}
