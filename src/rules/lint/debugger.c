/*
 * Lint/Debugger
 * https://docs.rubocop.org/rubocop/cops_lint.html#lintdebugger
 */

#include "rules/rule.h"
#include <string.h>
#include <stdlib.h>

// Rule definition
rule_t lint_debugger_rule = {
    .name = "Lint/Debugger",
    .category = CATEGORY_LINT,
    .enabled = true,
    .handlers = {[PM_CALL_NODE] = check_call}};

bool check_call(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics)
{
    return true;
}
