#include <string.h>
#include <stdlib.h>

#include "rules/rule.h"

// Rule definition
rule_t lint_debugger_rule = {
    .handlers = {[PM_CALL_NODE] = check_call}};

bool check_call(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics)
{
    return true;
}
