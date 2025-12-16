#ifndef LEUKOCYTE_RULE_MANAGER_H
#define LEUKOCYTE_RULE_MANAGER_H

#include "rules/rule.h"

// Initialize rules
void init_rules();

// Visit node for rule checking
bool visit_node(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg);

#endif /* LEUKOCYTE_RULE_MANAGER_H */
