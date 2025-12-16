#include <stdlib.h>
#include <string.h>

#include "rules/rule.h"
#include "rule_registry.h"

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
            rule->handlers[type]((pm_node_t *)node, visit_data->parser, visit_data->diagnostics, visit_data->cfg);
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

    const rule_registry_entry_t *registry = get_rule_registry();
    size_t registry_count = get_rule_registry_count();

    for (size_t i = 0; i < registry_count; i++)
    {
        rule_t *r = registry[i].rule;
        for (size_t node = 0; node < PM_NODE_TYPE_COUNT; node++)
        {
            if (!r->handlers[node])
                continue;

            size_t cur = rules_count_by_type[node];
            rule_t **newarr = realloc(rules_by_type[node], (cur + 1) * sizeof(rule_t *));
            if (!newarr)
                continue; // OOM: skip
            newarr[cur] = r;
            rules_by_type[node] = newarr;
            rules_count_by_type[node] = cur + 1;
        }
    }
}
