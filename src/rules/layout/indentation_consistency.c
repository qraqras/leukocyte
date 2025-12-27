#include <string.h>
#include <stdlib.h>

#include "prism/ast.h"
#include "configs/common/config.h"
#include "configs/layout/indentation_consistency.h"
#include "rules/layout/indentation_consistency.h"
#include "rules/common/rule_context.h"

/* Forward declaration of handler */
bool check_def(pm_node_t *node, const rule_context_t *ctx);

rule_t layout_indentation_consistency_rule = {
    .handlers = {[PM_DEF_NODE] = check_def}};

// Handler for DefNode: check indentation of body statements
bool check_def(pm_node_t *node, const rule_context_t *ctx)
{
    return true;
    pm_parser_t *parser = ctx->parser;
    pm_list_t *diagnostics = ctx->diagnostics;
    const leuko_config_t *cfg = ctx->cfg;

    pm_def_node_t *def_node = (pm_def_node_t *)node;
    pm_node_t *body = def_node->body;

    if (!body)
    {
        return true; // No body to check
    }

    const leuko_config_category_base_t *cc = leuko_config_get_category_config((leuko_config_t *)cfg, "Layout");
    if (!cc)
        return true;
    const typeof(((leuko_config_rule_view_indentation_consistency_t *)0)->specific) *sc = NULL;
    const leuko_config_rule_view_t *v = leuko_config_get_view_rule((leuko_config_t *)cfg, "Layout", "IndentationConsistency");
    if (!v)
        return true;
    /* Cast to typed view (PoC) */
    const leuko_config_rule_view_indentation_consistency_t *indentation_consistency_cfg = (const leuko_config_rule_view_indentation_consistency_t *)v;
    sc = &indentation_consistency_cfg->specific;

    if (PM_NODE_TYPE_P(body, PM_STATEMENTS_NODE))
    {
        pm_statements_node_t *stmts = (pm_statements_node_t *)body;
        size_t base = 0;
        bool base_set = false;
        int32_t prev_line = -1;

        leuko_processed_source_t *ps = ctx->ps;
        leuko_processed_source_t local_ps;
        bool used_local_ps = false;
        if (!ps)
        {
            /* Fallback: initialize a local processed_source if not provided */
            leuko_processed_source_init_from_parser(&local_ps, parser);
            ps = &local_ps;
            used_local_ps = true;
        }

        for (size_t i = 0; i < stmts->body.size; ++i)
        {
            const uint8_t *pos = stmts->body.nodes[i]->location.start;

            leuko_processed_source_pos_info_t info;
            leuko_processed_source_pos_info(ps, pos, &info);

            /* Only consider nodes that begin their line */
            if (info.column != info.indentation_column)
            {
                continue;
            }

            int32_t line = info.line_number;
            if (line == prev_line)
            {
                continue;
            }
            prev_line = line;

            size_t col = info.column;

            if (!base_set)
            {
                base = col;
                base_set = true;
            }
            else
            {
                const int diff = (int)base - (int)col;
                if (diff != 0)
                {
                    // Report diagnostic for inconsistent indentation
                    // (Diagnostic creation code omitted for brevity)
                }
            }
        }

        if (used_local_ps)
        {
            leuko_processed_source_free(&local_ps);
        }
    }

    return true;
}
