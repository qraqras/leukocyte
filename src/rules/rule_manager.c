#include <stdlib.h>
#include <string.h>

#include "rules/rule.h"
#include "rule_registry.h"

/* Array of rules by node type */
rule_t **rules_by_type[PM_NODE_TYPE_COUNT];
size_t rules_count_by_type[PM_NODE_TYPE_COUNT];

/* Data passed to visitor */
typedef struct
{
    pm_parser_t *parser;
    pm_list_t *diagnostics;
    config_t *cfg;
} visit_data_t;

/**
 * @brief Visitor function called for each node during AST traversal.
 * @param node Pointer to the current AST node
 * @param data Pointer to visit_data_t structure
 * @return true to continue visiting, false to stop
 */
bool node_visitor(const pm_node_t *node, void *data)
{
    visit_data_t *visit_data = (visit_data_t *)data;
    pm_node_type_t type = node->type;

    if (rules_count_by_type[type] > 0)
    {
        for (size_t i = 0; i < rules_count_by_type[type]; i++)
        {
            rule_t *rule = rules_by_type[type][i];
            rule->handlers[type]((pm_node_t *)node, visit_data->parser, visit_data->diagnostics, visit_data->cfg);
        }
    }

    return true;
}

/**
 * @brief Visit a node and apply relevant rules.
 * @param node Pointer to the AST node
 * @param parser Pointer to the parser
 * @param diagnostics Pointer to the diagnostics list
 * @param cfg Pointer to the configuration
 * @return true on success, false on failure
 */
bool visit_node(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg)
{
    visit_data_t data = {parser, diagnostics, cfg};
    pm_visit_node(node, node_visitor, &data);
    return true;
}

/**
 * @brief Initialize the rule manager by populating rules by node type.
 * @param void
 */
void init_rules(void)
{
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
            {
                continue;
            }

            size_t cur = rules_count_by_type[node];
            rule_t **newarr = realloc(rules_by_type[node], (cur + 1) * sizeof(rule_t *));
            if (!newarr)
            {
                continue;
            }
            newarr[cur] = r;
            rules_by_type[node] = newarr;
            rules_count_by_type[node] = cur + 1;
        }
    }
}
