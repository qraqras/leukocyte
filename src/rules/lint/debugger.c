#include <string.h>
#include <stdlib.h>

#include "rules/rule.h"

// Rule definition
rule_t lint_debugger_rule = {
    .handlers = {[PM_CALL_NODE] = check_call}};

bool check_call(pm_node_t *node, const rule_context_t *ctx)
{
    /* Example: use ctx->diagnostics or ctx->parser if needed */
    (void)ctx;
    return true;
}
